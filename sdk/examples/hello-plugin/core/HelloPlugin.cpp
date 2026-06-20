#include "HelloPlugin.h"

void HelloPlugin::OnInstall(IModuleContext& /*ctx*/)
{
    // Logging is dependency-free: formatted in-plugin via std::format, then
    // handed to the host across the POD log sink.
    Log::Info(
        "[HelloPlugin] Installed — built against the FrameLift SDK (ABI v{}.{}.{}).", FRAMELIFT_MODULE_ABI_MAJOR,
        FRAMELIFT_MODULE_ABI_MINOR, FRAMELIFT_MODULE_ABI_PATCH
    );
}
