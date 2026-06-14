#include <framelift/Log.h>
#include <string>

// Per-translation-unit sink pointer. Each plugin DLL (and the host) compiles its
// own copy of this TU; the host installs the sink into each via framelift_set_log_sink.

namespace
{
Log::SinkFn g_sink = nullptr;
}

void Log::SetSink(const SinkFn fn)
{
    g_sink = fn;
}

void Log::Emit(const Level level, const std::string& msg)
{
    if (g_sink)
    {
        g_sink(static_cast<int>(level), msg.c_str());
    }
}
