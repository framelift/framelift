#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

enum class SysGpuScope
{
    Unavailable,
    App,
    Device,
};

// A single snapshot of process-level OS performance counters.
// CPU/memory always describe THIS process (FrameLift). GPU is app-scoped when
// the OS exposes per-process/client counters; otherwise it may be a labeled
// device-wide fallback.
struct SysSample
{
    double cpuPercent = 0.0; // CPU usage, normalized across all cores (0-100)
    uint64_t memBytes = 0;   // resident memory / working set, in bytes
    double gpuPercent = 0.0; // best-effort GPU utilization (0-100)
    bool gpuValid = false;   // false → GPU usage unavailable, display "N/A"
    SysGpuScope gpuScope = SysGpuScope::Unavailable;
};

#if !defined(_WIN32)
struct DrmFdinfoCounters
{
    std::string clientKey;
    uint64_t engineNs = 0;
    bool valid = false;
};

DrmFdinfoCounters ParseDrmFdinfoCounters(std::string_view contents, std::string_view fallbackKey);
uint64_t TotalUniqueDrmEngineNs(const std::vector<DrmFdinfoCounters>& samples);
#endif

// Cross-platform sampler for process CPU / memory / GPU usage.
//
// Self-contained in the Benchmark plugin (SDK helper sources are compiled
// per-plugin, so nothing here is shared). CPU and GPU are rate counters that
// need two readings: the sampler caches the previous reading and computes the
// delta on each Sample(), so callers should poll at a steady cadence (~1/s).
class SysSampler
{
public:
    SysSampler();
    ~SysSampler();

    SysSampler(const SysSampler&) = delete;
    SysSampler& operator=(const SysSampler&) = delete;

    // Reads the current counters and returns a normalized snapshot.
    // The first call establishes a baseline for the rate counters (CPU/GPU),
    // so its cpuPercent/gpuPercent may read 0 until the next call.
    SysSample Sample();

    // Opaque platform-specific state, defined only in SysStats.cpp. Public so
    // file-local helpers in the .cpp can name SysSampler::Impl; still opaque to
    // callers since it has no definition in this header.
    struct Impl;

private:
    Impl* impl_ = nullptr; // PDH handles, previous CPU times, core count, ...
};
