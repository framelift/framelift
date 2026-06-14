// Cross-plugin service interfaces. Cross-plugin communication is events-first
// (see <framelift/Events.h>); a service interface exists only where a synchronous
// query is genuinely needed — currently just IHistory.
#pragma once
#include <framelift/services/IHistory.h>
