#include "HelloPlugin.h"

void HelloPlugin::OnInstall(IModuleContext& /*ctx*/)
{
    // Logging is dependency-free: formatted in-plugin via std::format, then
    // handed to the host across the POD log sink.
    Log::Info("[HelloPlugin] Installed — built against the FrameLift SDK (ABI version {}).", FRAMELIFT_ABI_VERSION);
}
