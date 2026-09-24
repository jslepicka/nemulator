module;
#include <Windows.h>
#include <stdio.h>

export module nemulator.std;
export import std;

export {
    using std::int8_t;
    using std::int16_t;
    using std::int32_t;
    using std::int64_t;

    using std::uint8_t;
    using std::uint16_t;
    using std::uint32_t;
    using std::uint64_t;
}

export template <typename T, auto method>
auto thunk(void * const ctx, auto... args)
{
    return (static_cast<T*>(ctx)->*method)(args...);
}

export bool debugger_present = IsDebuggerPresent();
export void ods(const char *message, ...)
{
    #ifdef _DEBUG
    if (debugger_present) {
        static char buf[128];
        va_list args;
        va_start(args, message);
        vprintf(message, args);
    }
    #endif
}

export consteval uint64_t broadcast8to64(uint8_t b)
{
    return b * 0x0101010101010101ULL;
}