#pragma once

// ── FrameLift plugin ABI rules ─────────────────────────────────────────────────────
//
// These rules apply to every type that crosses the host↔plugin DLL boundary.
// Violating them causes undefined behavior, crashes, or silent data corruption
// when host and plugin are built with different compilers or runtimes.
//
// 1. INTERFACES ONLY at the boundary.
//    Every exchanged object is a pure abstract class: single inheritance, only
//    virtual methods, no data members, no inline non-virtual logic that touches
//    object layout.  Declare methods noexcept; exceptions must not propagate
//    across a DLL boundary.
//
// 2. POD-ONLY signatures.
//    No std::string, std::vector, std::function, std::variant, std::optional,
//    std::unique_ptr, or by-value non-trivial structs in virtual method parameters
//    or return types.  Use instead:
//      - const char*     for string inputs (null-terminated, caller retains ownership)
//      - int Get(char*, int)  pattern for string outputs (returns byte count excl. NUL)
//      - enumeration callback  for collection outputs: void(*)(const T*, void*), void* ud
//      - function pointer + void* userdata  instead of std::function
//      - out-pointer parameter  for returned non-trivial values
//      - C POD structs  for exchanged data (fixed-size char arrays, numeric fields)
//
// 3. EXPLICIT IDs, not typeid.
//    Every interface declares:
//        static constexpr const char* InterfaceId = "framelift.IMyInterface";
//    Every POD event declares:
//        static constexpr const char* EventId = "framelift.MyEvent";
//    Service lookup and pub/sub are keyed on these string constants.
//    Never use typeid(T).name() across a DLL boundary.
//
// 4. STRING LIFETIME documented per direction.
//      host → plugin:  const char* params are valid only for the duration of the call.
//      plugin → host:  same.
//      Subscribers:    event const char* fields are valid only during the callback.
//    Plugins that need to outlive the call must copy into their own storage.
//
// 5. ABI VERSION (major.minor.patch — sdk/include/framelift/ModuleABI.h).
//    The loader accepts a plugin iff plugin.abiMajor == host.abiMajor and
//    plugin.abiMinor <= host.abiMinor, checked before any vtable is touched.
//      MAJOR — any breaking change: removing/reordering virtual methods,
//              changing a signature, OR appending to a host-CALLED plugin
//              interface (IModule, IRenderable) or framelift_* export — the host
//              would call a new slot on an older, shorter plugin vtable.
//      MINOR — backward-compatible additions to host-PROVIDED surface:
//              appending a method to IModuleContext, a new service interface, a
//              new optional export.  Old plugins keep loading.
//      PATCH — ABI-neutral fixes; carried and logged but not gated.
//    FrameLiftPackageInfo is itself part of the ABI: only a MAJOR bump may change its
//    layout.
//
// ── Helper macros ─────────────────────────────────────────────────────────────

// Declare a non-copyable, non-movable interface base.
// Use at the top of every boundary interface class body.
#define FRAMELIFT_INTERFACE(ClassName)                                                                                      \
    ClassName(const ClassName&) = delete;                                                                              \
    ClassName& operator=(const ClassName&) = delete;                                                                   \
    ClassName(ClassName&&) = delete;                                                                                   \
    ClassName& operator=(ClassName&&) = delete
