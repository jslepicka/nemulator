module;
#include <Windows.h>
#include <stdio.h>

export module nemulator.std;
export import std;
using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;

using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;

export template <typename T, auto method>
auto thunk(void * const ctx, auto... args)
{
    return (static_cast<T*>(ctx)->*method)(args...);
}

export void ods(const char *message, ...)
{
    static char buf[128];
    va_list args;
    va_start(args, message);
    vsprintf(buf, message, args);
    OutputDebugString(buf);
}