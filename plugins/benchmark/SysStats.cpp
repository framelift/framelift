#include "SysStats.h"

#include <algorithm>
#include <chrono>

// ── Windows ─────────────────────────────────────────────────────────────────
#if defined(_WIN32)

#include <windows.h>

#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>

#include <string>
#include <string_view>
#include <vector>

struct SysSampler::Impl
{
    // CPU: process kernel+user time vs. wall time, normalized across cores.
    bool cpuPrimed = false;
    uint64_t prevCpu100ns = 0; // kernel+user, in 100-ns units
    std::chrono::steady_clock::time_point prevWall{};
    DWORD numProcessors = 1;

    // GPU: PDH wildcard query summed over this process's 3D engine instances.
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER counter = nullptr;
    bool gpuAvailable = false;
    bool gpuInitTried = false; // PDH query opened lazily on first SampleGpu
    std::string pidNeedle;     // "pid_<id>" — filters instances belonging to us
};

static uint64_t FileTimeToU64(const FILETIME& ft)
{
    ULARGE_INTEGER u;
    u.LowPart = ft.dwLowDateTime;
    u.HighPart = ft.dwHighDateTime;
    return u.QuadPart;
}

SysSampler::SysSampler() : impl_(new Impl)
{
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    impl_->numProcessors = std::max<DWORD>(1, si.dwNumberOfProcessors);

    impl_->pidNeedle = "pid_" + std::to_string(GetCurrentProcessId());
}

// Open the PDH GPU query on first use. Priming the wildcard GPU counter
// enumerates every GPU engine instance — hundreds of ms on the first call — so
// it is deferred out of the constructor (which runs at plugin load) to here,
// reached only once the benchmark overlay is actually sampling.
static void EnsureGpuQuery(SysSampler::Impl& s)
{
    if (s.gpuInitTried)
    {
        return;
    }
    s.gpuInitTried = true;

    if (PdhOpenQueryA(nullptr, 0, &s.query) == ERROR_SUCCESS)
    {
        const PDH_STATUS st =
            PdhAddCounterA(s.query, "\\GPU Engine(*engtype_3D)\\Utilization Percentage", 0, &s.counter);
        if (st == ERROR_SUCCESS)
        {
            // Prime the counter so the next Sample() has a delta to format.
            PdhCollectQueryData(s.query);
            s.gpuAvailable = true;
        }
        else
        {
            PdhCloseQuery(s.query);
            s.query = nullptr;
        }
    }
}

SysSampler::~SysSampler()
{
    if (impl_ && impl_->query)
    {
        PdhCloseQuery(impl_->query);
    }
    delete impl_;
}

static double SampleGpu(SysSampler::Impl& s)
{
    EnsureGpuQuery(s);
    if (!s.gpuAvailable || PdhCollectQueryData(s.query) != ERROR_SUCCESS)
    {
        return -1.0;
    }

    DWORD bufSize = 0;
    DWORD itemCount = 0;
    PDH_STATUS st = PdhGetFormattedCounterArrayA(s.counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, nullptr);
    if (st != PDH_MORE_DATA || bufSize == 0)
    {
        return -1.0;
    }

    std::vector<uint8_t> buf(bufSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_A*>(buf.data());
    if (PdhGetFormattedCounterArrayA(s.counter, PDH_FMT_DOUBLE, &bufSize, &itemCount, items) != ERROR_SUCCESS)
    {
        return -1.0;
    }

    double total = 0.0;
    for (DWORD i = 0; i < itemCount; ++i)
    {
        const auto& item = items[i];
        if (item.FmtValue.CStatus != PDH_CSTATUS_VALID_DATA || item.szName == nullptr)
        {
            continue;
        }
        // Instance names look like "pid_1234_luid_..._engtype_3D"; keep ours only.
        if (std::string_view(item.szName).find(s.pidNeedle) != std::string_view::npos)
        {
            total += item.FmtValue.doubleValue;
        }
    }
    return std::clamp(total, 0.0, 100.0);
}

SysSample SysSampler::Sample()
{
    SysSample out;
    Impl& s = *impl_;

    // ── Memory: working set ──────────────────────────────────────────────────
    PROCESS_MEMORY_COUNTERS pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        out.memBytes = pmc.WorkingSetSize;
    }

    // ── CPU: kernel+user delta over wall delta, normalized across cores ──────
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    if (GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser))
    {
        const uint64_t cpu100ns = FileTimeToU64(ftKernel) + FileTimeToU64(ftUser);
        const auto now = std::chrono::steady_clock::now();
        if (s.cpuPrimed)
        {
            const double wallSec = std::chrono::duration<double>(now - s.prevWall).count();
            const double cpuSec = static_cast<double>(cpu100ns - s.prevCpu100ns) / 1e7;
            if (wallSec > 0.0)
            {
                out.cpuPercent = std::clamp(cpuSec / wallSec / s.numProcessors * 100.0, 0.0, 100.0);
            }
        }
        s.prevCpu100ns = cpu100ns;
        s.prevWall = now;
        s.cpuPrimed = true;
    }

    // ── GPU: best-effort ─────────────────────────────────────────────────────
    const double gpu = SampleGpu(s);
    if (gpu >= 0.0)
    {
        out.gpuPercent = gpu;
        out.gpuValid = true;
        out.gpuScope = SysGpuScope::App;
    }

    return out;
}

// ── Linux (and other POSIX) ─────────────────────────────────────────────────
#else

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <string_view>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include <dlfcn.h>

#include <framelift/Log.h>

// Minimal NVML surface, declared locally so the build needs no nvml.h and no
// link-time dependency on libnvidia-ml. Symbols are resolved at runtime via
// dlopen/dlsym; the proprietary NVIDIA driver ships libnvidia-ml.so(.1).
namespace
{
using NvmlDevice = void*;

struct NvmlUtilization
{
    unsigned int gpu;    // percent of time the GPU was busy
    unsigned int memory; // percent of time GPU memory was read/written
};

struct NvmlProcessUtilizationSample
{
    unsigned int pid;
    unsigned long long timeStamp;
    unsigned int smUtil;
    unsigned int memUtil;
    unsigned int encUtil;
    unsigned int decUtil;
};

using NvmlReturn = int; // NVML_SUCCESS == 0
constexpr NvmlReturn kNvmlSuccess = 0;
constexpr NvmlReturn kNvmlNotSupported = 3;
constexpr NvmlReturn kNvmlNotFound = 6;
constexpr NvmlReturn kNvmlInsufficientSize = 7;

using FnNvmlInit = NvmlReturn (*)();
using FnNvmlShutdown = NvmlReturn (*)();
using FnNvmlGetHandleByIndex = NvmlReturn (*)(unsigned int, NvmlDevice*);
using FnNvmlGetUtilizationRates = NvmlReturn (*)(NvmlDevice, NvmlUtilization*);
using FnNvmlGetProcessUtilization =
    NvmlReturn (*)(NvmlDevice, NvmlProcessUtilizationSample*, unsigned int*, unsigned long long);
} // namespace

struct SysSampler::Impl
{
    bool cpuPrimed = false;
    uint64_t prevJiffies = 0; // utime + stime, in clock ticks
    std::chrono::steady_clock::time_point prevWall{};
    long ticksPerSec = 100;
    long numProcessors = 1;
    bool drmPrimed = false;
    uint64_t prevDrmEngineNs = 0;
    std::chrono::steady_clock::time_point prevDrmWall{};

    // NVML: loaded lazily on first GPU sample. nvmlReady gates use; nvmlTried
    // ensures we attempt the dlopen + init exactly once.
    void* nvmlHandle = nullptr;
    bool nvmlTried = false;
    bool nvmlReady = false;
    NvmlDevice nvmlDev = nullptr;
    FnNvmlShutdown nvmlShutdown = nullptr;
    FnNvmlGetUtilizationRates nvmlGetUtilizationRates = nullptr;
    FnNvmlGetProcessUtilization nvmlGetProcessUtilization = nullptr;
    unsigned long long nvmlLastProcessTimestamp = 0;
};

SysSampler::SysSampler() : impl_(new Impl)
{
    impl_->ticksPerSec = std::max<long>(1, sysconf(_SC_CLK_TCK));
    impl_->numProcessors = std::max<long>(1, sysconf(_SC_NPROCESSORS_ONLN));
}

SysSampler::~SysSampler()
{
    if (impl_->nvmlReady && impl_->nvmlShutdown)
    {
        impl_->nvmlShutdown();
    }
    if (impl_->nvmlHandle)
    {
        dlclose(impl_->nvmlHandle);
    }
    delete impl_;
}

namespace
{
struct DrmEngineSnapshot
{
    uint64_t engineNs = 0;
    bool valid = false;
};

std::string_view Trim(std::string_view s)
{
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())) != 0)
    {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())) != 0)
    {
        s.remove_suffix(1);
    }
    return s;
}

bool StartsWith(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

bool ParseUnsignedPrefix(std::string_view s, uint64_t& out)
{
    s = Trim(s);
    if (s.empty() || std::isdigit(static_cast<unsigned char>(s.front())) == 0)
    {
        return false;
    }

    size_t n = 0;
    while (n < s.size() && std::isdigit(static_cast<unsigned char>(s[n])) != 0)
    {
        ++n;
    }

    const std::string digits(s.substr(0, n));
    char* end = nullptr;
    const unsigned long long value = std::strtoull(digits.c_str(), &end, 10);
    if (end == digits.c_str())
    {
        return false;
    }
    out = static_cast<uint64_t>(value);
    return true;
}

std::string ReadWholeFile(const std::string& path)
{
    FILE* f = std::fopen(path.c_str(), "r");
    if (!f)
    {
        return {};
    }

    std::string contents;
    char chunk[1024];
    size_t n = 0;
    while ((n = std::fread(chunk, 1, sizeof(chunk), f)) > 0)
    {
        contents.append(chunk, n);
    }
    std::fclose(f);
    return contents;
}
} // namespace

DrmFdinfoCounters ParseDrmFdinfoCounters(const std::string_view contents, const std::string_view fallbackKey)
{
    std::string driver;
    std::string pdev;
    std::string clientId;
    uint64_t engineNs = 0;
    bool hasEngine = false;

    size_t pos = 0;
    while (pos < contents.size())
    {
        const size_t end = contents.find('\n', pos);
        const std::string_view line =
            end == std::string_view::npos ? contents.substr(pos) : contents.substr(pos, end - pos);
        pos = end == std::string_view::npos ? contents.size() : end + 1;

        const size_t colon = line.find(':');
        if (colon == std::string_view::npos)
        {
            continue;
        }
        const std::string_view key = Trim(line.substr(0, colon));
        const std::string_view value = Trim(line.substr(colon + 1));
        if (key == "drm-driver")
        {
            driver = std::string(value);
        }
        else if (key == "drm-pdev")
        {
            pdev = std::string(value);
        }
        else if (key == "drm-client-id")
        {
            clientId = std::string(value);
        }
        else if (StartsWith(key, "drm-engine-"))
        {
            uint64_t ns = 0;
            if (ParseUnsignedPrefix(value, ns))
            {
                engineNs += ns;
                hasEngine = true;
            }
        }
    }

    DrmFdinfoCounters out;
    out.engineNs = engineNs;
    out.valid = hasEngine;
    if (!driver.empty() || !pdev.empty() || !clientId.empty())
    {
        out.clientKey = driver + "|" + pdev + "|" + clientId;
    }
    else
    {
        out.clientKey = std::string(fallbackKey);
    }
    return out;
}

uint64_t TotalUniqueDrmEngineNs(const std::vector<DrmFdinfoCounters>& samples)
{
    std::unordered_map<std::string, uint64_t> byClient;
    for (const auto& sample : samples)
    {
        if (!sample.valid)
        {
            continue;
        }
        auto& value = byClient[sample.clientKey];
        value = std::max(value, sample.engineNs);
    }

    uint64_t total = 0;
    for (const auto& [_, value] : byClient)
    {
        total += value;
    }
    return total;
}

// Returns utime+stime in clock ticks from /proc/self/stat, or 0 on failure.
static uint64_t ReadProcessJiffies()
{
    FILE* f = std::fopen("/proc/self/stat", "r");
    if (!f)
    {
        return 0;
    }
    std::string contents;
    char chunk[512];
    size_t n;
    while ((n = std::fread(chunk, 1, sizeof(chunk), f)) > 0)
    {
        contents.append(chunk, n);
    }
    std::fclose(f);

    // comm (field 2) is wrapped in parens and may contain spaces/parens; the
    // numeric fields begin after the LAST ')'. utime/stime are fields 14/15,
    // i.e. tokens 11/12 (0-based) of the remainder.
    const auto rparen = contents.rfind(')');
    if (rparen == std::string::npos)
    {
        return 0;
    }

    uint64_t utime = 0, stime = 0;
    int idx = 0;
    const char* p = contents.c_str() + rparen + 1;
    while (*p)
    {
        while (*p == ' ')
        {
            ++p;
        }
        if (!*p)
        {
            break;
        }
        const char* tok = p;
        while (*p && *p != ' ')
        {
            ++p;
        }
        // Tokens after the last ')' are 0-based: idx 11 = utime, 12 = stime.
        if (idx == 11)
        {
            utime = std::strtoull(tok, nullptr, 10);
        }
        else if (idx == 12)
        {
            stime = std::strtoull(tok, nullptr, 10);
            break;
        }
        ++idx;
    }
    return utime + stime;
}

static uint64_t ReadResidentBytes()
{
    FILE* f = std::fopen("/proc/self/statm", "r");
    if (!f)
    {
        return 0;
    }
    unsigned long long total = 0, resident = 0;
    const int got = std::fscanf(f, "%llu %llu", &total, &resident);
    std::fclose(f);
    if (got < 2)
    {
        return 0;
    }
    return resident * static_cast<uint64_t>(sysconf(_SC_PAGESIZE));
}

static DrmEngineSnapshot ReadDrmEngineSnapshot()
{
    DIR* dir = opendir("/proc/self/fdinfo");
    if (!dir)
    {
        return {};
    }

    std::vector<DrmFdinfoCounters> samples;
    while (const dirent* ent = readdir(dir))
    {
        const char* name = ent->d_name;
        if (!name || std::isdigit(static_cast<unsigned char>(name[0])) == 0)
        {
            continue;
        }
        const std::string path = std::string("/proc/self/fdinfo/") + name;
        const std::string contents = ReadWholeFile(path);
        if (contents.empty())
        {
            continue;
        }
        samples.push_back(ParseDrmFdinfoCounters(contents, path));
    }
    closedir(dir);

    DrmEngineSnapshot out;
    out.engineNs = TotalUniqueDrmEngineNs(samples);
    out.valid = !samples.empty() && std::any_of(
                                        samples.begin(), samples.end(),
                                        [](const DrmFdinfoCounters& sample)
                                        {
                                            return sample.valid;
                                        }
                                    );
    return out;
}

// Best-effort GPU utilization via sysfs (AMD exposes gpu_busy_percent).
// Returns -1 when unavailable.
static double ReadGpuBusyPercent()
{
    FILE* f = std::fopen("/sys/class/drm/card0/device/gpu_busy_percent", "r");
    if (!f)
    {
        return -1.0;
    }
    int pct = -1;
    const int got = std::fscanf(f, "%d", &pct);
    std::fclose(f);
    if (got < 1 || pct < 0)
    {
        return -1.0;
    }
    return std::clamp(static_cast<double>(pct), 0.0, 100.0);
}

// Load libnvidia-ml and bind the symbols we need on first use. Mirrors the
// Windows EnsureGpuQuery deferral: nvmlInit enumerates devices and is slow, so
// it is kept out of the constructor (which runs at plugin load) and reached only
// once the overlay is actually sampling. Stays silent when the library is simply
// absent (the normal non-NVIDIA case); warns once if it is present but fails.
static void EnsureNvml(SysSampler::Impl& s)
{
    if (s.nvmlTried)
    {
        return;
    }
    s.nvmlTried = true;

    s.nvmlHandle = dlopen("libnvidia-ml.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!s.nvmlHandle)
    {
        s.nvmlHandle = dlopen("libnvidia-ml.so", RTLD_LAZY | RTLD_LOCAL);
    }
    if (!s.nvmlHandle)
    {
        return; // No NVIDIA driver/NVML installed — fall back to "N/A".
    }

    auto init = reinterpret_cast<FnNvmlInit>(dlsym(s.nvmlHandle, "nvmlInit_v2"));
    auto getHandle = reinterpret_cast<FnNvmlGetHandleByIndex>(dlsym(s.nvmlHandle, "nvmlDeviceGetHandleByIndex_v2"));
    s.nvmlShutdown = reinterpret_cast<FnNvmlShutdown>(dlsym(s.nvmlHandle, "nvmlShutdown"));
    s.nvmlGetUtilizationRates =
        reinterpret_cast<FnNvmlGetUtilizationRates>(dlsym(s.nvmlHandle, "nvmlDeviceGetUtilizationRates"));
    s.nvmlGetProcessUtilization =
        reinterpret_cast<FnNvmlGetProcessUtilization>(dlsym(s.nvmlHandle, "nvmlDeviceGetProcessUtilization"));

    if (!init || !getHandle || !s.nvmlShutdown || !s.nvmlGetUtilizationRates)
    {
        Log::Warn("NVML loaded but required symbols are missing; GPU usage unavailable");
        return;
    }

    if (init() != 0 || getHandle(0, &s.nvmlDev) != 0 || !s.nvmlDev)
    {
        Log::Warn("NVML init/device query failed; GPU usage unavailable");
        return;
    }

    s.nvmlReady = true;
}

// Per-process SM utilization via NVML. Covers proprietary NVIDIA drivers that
// do not expose DRM engine counters in /proc/self/fdinfo. Returns -1 only when
// the API is unavailable/unsupported; otherwise returns this process's current
// sample, including 0 when NVML has no non-zero sample for us.
static double SampleNvmlProcessGpu(SysSampler::Impl& s)
{
    EnsureNvml(s);
    if (!s.nvmlReady || !s.nvmlGetProcessUtilization)
    {
        return -1.0;
    }

    unsigned int count = 0;
    NvmlReturn result = s.nvmlGetProcessUtilization(s.nvmlDev, nullptr, &count, s.nvmlLastProcessTimestamp);
    if (result == kNvmlNotFound)
    {
        return 0.0;
    }
    if (result == kNvmlNotSupported)
    {
        return -1.0;
    }
    if (result != kNvmlInsufficientSize || count == 0)
    {
        return -1.0;
    }

    std::vector<NvmlProcessUtilizationSample> samples(count);
    result = s.nvmlGetProcessUtilization(s.nvmlDev, samples.data(), &count, s.nvmlLastProcessTimestamp);
    if (result == kNvmlNotFound)
    {
        return 0.0;
    }
    if (result != kNvmlSuccess)
    {
        return -1.0;
    }

    const unsigned int pid = static_cast<unsigned int>(getpid());
    unsigned int util = 0;
    unsigned long long newestTimestamp = s.nvmlLastProcessTimestamp;
    for (unsigned int i = 0; i < count && i < samples.size(); ++i)
    {
        const auto& sample = samples[i];
        newestTimestamp = std::max(newestTimestamp, sample.timeStamp);
        if (sample.pid == pid)
        {
            util = std::max(util, sample.smUtil);
        }
    }
    s.nvmlLastProcessTimestamp = newestTimestamp;
    return std::clamp(static_cast<double>(util), 0.0, 100.0);
}

// Whole-GPU utilization (device 0) via NVML. Returns -1 when unavailable.
static double SampleNvmlGpu(SysSampler::Impl& s)
{
    EnsureNvml(s);
    if (!s.nvmlReady)
    {
        return -1.0;
    }

    NvmlUtilization util{};
    if (s.nvmlGetUtilizationRates(s.nvmlDev, &util) != 0)
    {
        return -1.0;
    }
    return std::clamp(static_cast<double>(util.gpu), 0.0, 100.0);
}

SysSample SysSampler::Sample()
{
    SysSample out;
    Impl& s = *impl_;

    out.memBytes = ReadResidentBytes();

    const uint64_t jiffies = ReadProcessJiffies();
    const auto now = std::chrono::steady_clock::now();
    if (s.cpuPrimed)
    {
        const double wallSec = std::chrono::duration<double>(now - s.prevWall).count();
        const double cpuSec = static_cast<double>(jiffies - s.prevJiffies) / s.ticksPerSec;
        if (wallSec > 0.0)
        {
            out.cpuPercent = std::clamp(cpuSec / wallSec / s.numProcessors * 100.0, 0.0, 100.0);
        }
    }
    s.prevJiffies = jiffies;
    s.prevWall = now;
    s.cpuPrimed = true;

    const DrmEngineSnapshot drm = ReadDrmEngineSnapshot();
    if (drm.valid)
    {
        out.gpuValid = true;
        out.gpuScope = SysGpuScope::App;
        if (s.drmPrimed)
        {
            const double wallSec = std::chrono::duration<double>(now - s.prevDrmWall).count();
            const uint64_t deltaNs = drm.engineNs >= s.prevDrmEngineNs ? drm.engineNs - s.prevDrmEngineNs : 0;
            if (wallSec > 0.0)
            {
                const double gpuSec = static_cast<double>(deltaNs) / 1e9;
                out.gpuPercent = std::clamp(gpuSec / wallSec * 100.0, 0.0, 100.0);
            }
        }
        s.prevDrmEngineNs = drm.engineNs;
        s.prevDrmWall = now;
        s.drmPrimed = true;
    }
    else
    {
        double gpu = SampleNvmlProcessGpu(s);
        if (gpu >= 0.0)
        {
            out.gpuPercent = gpu;
            out.gpuValid = true;
            out.gpuScope = SysGpuScope::App;
        }
        // Device-wide fallbacks keep visibility but are labeled as such in the UI.
        else if ((gpu = ReadGpuBusyPercent()) >= 0.0)
        {
            out.gpuPercent = gpu;
            out.gpuValid = true;
            out.gpuScope = SysGpuScope::Device;
        }
        if (gpu < 0.0)
        {
            gpu = SampleNvmlGpu(s);
            if (gpu >= 0.0)
            {
                out.gpuPercent = gpu;
                out.gpuValid = true;
                out.gpuScope = SysGpuScope::Device;
            }
        }
    }

    return out;
}

#endif
