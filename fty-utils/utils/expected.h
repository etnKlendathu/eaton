#pragma once
#include <string>

template <typename T>
class Expected
{
public:
    Expected(const T& value):
        m_value(value)
    {}

    Expected(T&& value):
        m_value(std::move(value))
    {}

    ~Expected()
    {
        if (m_isError) {

        }
    }

    const T& value() const
    {
        return m_value;
    }

    const std::string& error() const
    {
        return m_error;
    }

    bool isValid() const
    {
        return m_isError;
    }

private:
    union {
        T           m_value;
        std::string m_error;
    };
    bool m_isError = false;
};
