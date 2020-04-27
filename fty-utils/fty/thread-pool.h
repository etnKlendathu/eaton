#pragma once

#include <condition_variable>
#include <deque>
#include <fty/event.h>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

template <typename>
struct MemberPointerTraits
{
};

template <class T, class U>
struct MemberPointerTraits<U T::*>
{
    using member_type = U;
};

class ThreadPool
{
public:
    class Task
    {
    public:
        fty::Event<> started;
        fty::Event<> stopped;

    public:
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        Task(Task&&)  = default;
        Task& operator=(Task&&) = default;

        void operator()()
        {
            m_func();
        }

    protected:
        template <typename Func, typename... Args>
        Task(Func&& fnc, Args&&... args)
            : m_func([f = std::move(fnc), cargs = std::make_tuple(std::forward<Args>(args)...)]() -> void {
                std::apply(std::move(f), std::move(cargs));
            })
        {
        }

    private:
        using TaskFunc = std::function<void()>;
        TaskFunc m_func;
        friend class ThreadPool;
    };

public:
    enum class Stop
    {
        WaitForTasks,
        Immedialy
    };

public:
    ThreadPool(size_t numThreads = std::thread::hardware_concurrency() - 1)
    {
        for (size_t i = 0; i < numThreads; ++i) {
            m_threads.emplace_back(std::thread([&]() {
                while (true) {
                    std::shared_ptr<Task> task;
                    {
                        std::unique_lock<std::mutex> lock(m_mutex);

                        m_cv.wait(lock, [&]() {
                            return !m_tasks.empty() || m_stop;
                        });

                        if (m_stop) {
                            return;
                        }

                        if (!m_tasks.empty()) {
                            task = std::move(m_tasks.front());
                            m_tasks.pop_front();
                        }
                    }
                    m_cv.notify_all();
                    if (task) {
                        task->started();
                        (*task)();
                        task->stopped();
                    }
                }
            }));
        }
    }

    ~ThreadPool()
    {
        stop(Stop::Immedialy);
    }

    void stop(Stop mode)
    {
        if (!m_stop) {
            {
                if (mode == Stop::WaitForTasks) {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [&]() {
                        return m_tasks.empty();
                    });
                }

                std::lock_guard<std::mutex> lock(m_mutex);
                m_stop = true;
            }
            m_cv.notify_all();

            for (std::thread& thread : m_threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename Func, typename... Args>
    std::enable_if_t<std::is_function_v<typename MemberPointerTraits<Func>::member_type>,
        std::shared_ptr<Task>>
    pushWorker(Func&& fnc, Args&&... args)
    {
        std::shared_ptr<Task> task;
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            task = std::shared_ptr<Task>(new Task(std::forward<Func>(fnc), std::forward<Args>(args)...));
            m_tasks.emplace_back(task);
        }
        m_cv.notify_all();
        return task;
    }

    void pushWorker(const std::shared_ptr<Task>& task)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.emplace_back(task);
        }
        m_cv.notify_all();
    }

private:
    std::vector<std::thread>          m_threads;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    bool                              m_stop = false;
    std::deque<std::shared_ptr<Task>> m_tasks;
};
