#pragma once

// Pure interface for directory-change monitoring.
// The onChange callback fires on the watcher's internal thread; callers must
// marshal to the main thread (e.g., via SDL event queue) before touching state.
class IDirWatcher
{
public:
    static constexpr const char* InterfaceId = "framelift.IDirWatcher";
    virtual ~IDirWatcher() = default;

    // Begin watching rootDir for CREATE / DELETE / MOVE changes.
    // onChange(ud) fires on the watcher's background thread — keep it brief and
    // thread-safe. Calling Watch() again replaces the current watch.
    // maxDepth: 0 = root only, N > 0 = up to N subdirectory levels, -1 = unlimited.
    virtual void Watch(const char* rootDir, void (*onChange)(void* ud), void* ud, int maxDepth = 0) noexcept = 0;

    // Stop the current watch. No-op if not currently watching.
    // Blocks until any in-flight callback has returned.
    virtual void Unwatch() noexcept = 0;
};