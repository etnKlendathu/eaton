#pragma once
#include <string>
#include <cassert>
#include "convert.h"

struct Unexpected
{
    std::string message;

    template<typename T>
    Unexpected& operator<<(const T& val)
    {
        message += fty::convert<std::string>(val);
        return *this;
    }
};

template <typename T>
class Expected
{
public:
    Expected(const T& value)
        : m_value(value)
    {
    }

    Expected(T&& value)
        : m_value(std::move(value))
    {
    }

    Expected(Unexpected&& unex)
        : m_error(std::move(unex.message))
        , m_isError(true)
    {
    }

    Expected(const Unexpected& unex)
        : m_error(unex.message)
        , m_isError(true)
    {
    }

    ~Expected()
    {
        if (m_isError) {
            m_error.~basic_string();
        } else {
            m_value.~T();
        }
    }

    const T& value() const
    {
        assert(!m_isError);
        return m_value;
    }

    const std::string& error() const
    {
        assert(m_isError);
        return m_error;
    }

    bool isValid() const noexcept
    {
        return m_isError;
    }

    operator bool() const noexcept
    {
        return !m_isError;
    }

    const T& operator*() const noexcept
    {
        assert(!m_isError);
        return m_value;
    }

    T& operator*() noexcept
    {
        assert(!m_isError);
        return m_value;
    }

    const T* operator->() const noexcept
    {
        assert(!m_isError);
        return &m_value;
    }

    T* operator->() noexcept
    {
        assert(!m_isError);
        return &m_value;
    }

private:
    union {
        T           m_value;
        std::string m_error;
    };
    bool m_isError = false;
};

inline Unexpected unexpected(const std::string& error = {})
{
    return {error};
}
