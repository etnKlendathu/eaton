#include <fty/thread-pool.h>

using namespace std::chrono_literals;

class Job
{
public:
    void run()
    {
        std::cerr << "Job thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(100ms);
        std::cerr << "Run job" << std::endl;
    }
};

class Job2: public ThreadPool::Task
{
public:
    Job2():
        ThreadPool::Task(&Job2::run, this)
    {}

    void run()
    {
        std::cerr << "Job2 thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(100ms);
        std::cerr << "Run job2" << std::endl;
    }
};

class Job3
{
public:
    void run()
    {
        std::cerr << "Job 3 thread " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(3s);
        std::cerr << "Run 3 job" << std::endl;
    }
};

int main()
{
    std::cerr << "current thread " << std::this_thread::get_id() << std::endl;

    int count = 0;
    fty::Slot<> slot([&]() {
        std::cerr << "task was finished\n";
        ++count;
    });
//    fty::Slot<> slot2([&]() {
//        std::cerr << "task 2 was finished\n";
//        ++count;
//    });

//    for(int i = 0; i < 40; ++i) {
//        ThreadPool pool;
//        Job        job;

//        auto task = pool.pushWorker(&Job::run, &job);
//        task->stopped.connect(slot);

//        std::shared_ptr<Job2> job2 = std::make_shared<Job2>();
//        job2->stopped.connect(slot2);
//        pool.pushWorker(job2);

//        pool.stop(ThreadPool::Stop::WaitForTasks);
//    }

//    std::cerr << "count " << count << std::endl;

    ThreadPool pool;
    Job3        job;

    auto task = pool.pushWorker(&Job3::run, &job);
    task->stopped.connect(slot);
    std::cerr << "begin wait\n";
    task->stopped.wait();
    std::cerr << "end wait\n";

    return 0;
}
