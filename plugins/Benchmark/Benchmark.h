#pragma once

#include <framelift/core.h>
#include <framelift/ui.h>

#include "SysStats.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>

// Running min/avg/max aggregate over one sampled metric. Folded once per
// sample while a benchmark run is recording; reset between runs.
struct Stat
{
    double min = 0.0;
    double max = 0.0;
    double sum = 0.0;
    uint64_t count = 0;

    void Reset()
    {
        *this = Stat{};
    }

    void Add(const double v)
    {
        min = count == 0 ? v : std::min(min, v);
        max = count == 0 ? v : std::max(max, v);
        sum += v;
        ++count;
    }

    [[nodiscard]] double Avg() const
    {
        return count > 0 ? sum / static_cast<double>(count) : 0.0;
    }
};

// Performance benchmark overlay (F10 to toggle).
// Shows live process performance — CPU %, memory, GPU % with running
// min/avg/max — alongside player playback stats (dropped/mistimed frames and
// the active hardware/software decoder) in a dark semi-transparent panel
// anchored to the top-right corner. A "Load file" button starts a chosen video
// from position 0 for a reproducible run; an optional duration limit pauses
// playback and freezes the results once playback reaches the configured length.
// Polled stats refresh once per second; idle state is pushed via OnMediaEvent.
class Benchmark final : public SafeRenderable, public PluginBase
{
public:
    const char* PluginName() const override
    {
        return "Benchmark";
    }

    void Toggle()
    {
        open_ = !open_;
    }

    [[nodiscard]] bool IsOpen() const
    {
        return open_;
    }

    void HandleMediaEvent(const MediaEvent& event) override;

    void OnRender(UIContext& ctx) override;

protected:
    std::vector<framelift::SettingsField> SettingsFields() override;
    std::vector<framelift::Keybind> Keybinds() override;
    void OnInstall(IPluginContext& ctx) override;
    void RenderSettings(UIContext& ctx) override;

private:
    void RequestRefresh();
    void StartRun(const char* path); // load `path` from 0 and begin recording
    void ResetStats();

    bool open_ = false;
    std::string toggleBenchmarkKey_ = "F10";

    SysSampler sampler_;
    SysSample sys_;

    // ── Cached player stats ─────────────────────────────────────────────────────
    bool isIdle_ = true;
    std::string hwDec_ = "N/A";
    int64_t dropped_ = 0;
    int64_t mistimed_ = 0;

    // ── Benchmark run state ─────────────────────────────────────────────────────
    Stat cpuStat_;
    Stat memStat_; // bytes
    Stat gpuStat_;
    bool accumulating_ = false; // folding samples into the Stats
    bool complete_ = false;     // duration limit reached — results frozen
    double timePos_ = 0.0;      // latest playback position (seconds)

    // ── Settings ────────────────────────────────────────────────────────────────
    bool limitDuration_ = false;
    float benchmarkDuration_ = 30.f; // seconds

    std::chrono::steady_clock::time_point lastRefresh_{};
    static constexpr double refreshInterval = 1.0; // seconds
};
