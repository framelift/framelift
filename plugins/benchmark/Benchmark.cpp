#include "Benchmark.h"
#include "BenchmarkSettings.h"

#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "Version.h"
#include <framelift/platform.h>

Benchmark::Benchmark() = default;

Benchmark::~Benchmark() = default;

std::vector<framelift::Keybind> Benchmark::Keybinds()
{
    return {
        {"Toggle benchmark overlay", "toggleBenchmark", &toggleBenchmarkKey_, "F10", [this]
         {
             Toggle();
         }}
    };
}

void Benchmark::LoadSettings(IModuleSettings& ps)
{
    limitDuration_ = ps.GetBool("limitDuration", false);
    benchmarkDuration_ = ps.GetFloat("benchmarkDuration", 30.0f);
}

void Benchmark::SaveSettings(IModuleSettings& ps)
{
    ps.SetBool("limitDuration", limitDuration_);
    ps.SetFloat("benchmarkDuration", benchmarkDuration_);
}

void Benchmark::OnInstall(IModuleContext& ctx)
{
    if (auto* pages = ctx.GetService<ISettingsPageRegistry>())
    {
        settingsPage_ = std::make_unique<BenchmarkSettings>(*this);
        pages->RegisterSettingsPage(
            "benchmark", "Benchmark", "qrc:/qt/qml/FrameLift/Plugins/Benchmark/BenchmarkSettings.qml",
            settingsPage_.get(), 330
        );
    }
    if (auto* props = ctx.GetService<IMediaProperties>())
    {
        props->ObserveProperty(PlayerProperty::TimePos);
    }
    // The graphics backend is selected once at startup and never switches, so read it once.
    if (auto* gfx = ctx.GetService<IGraphicsInfo>())
    {
        char buf[64] = {};
        gfx->GetBackendName(buf, sizeof(buf));
        gfxBackend_ = buf;
        gpuIsNvidia_ = gfx->HasNvidiaAdapter();
    }
    frameSamples_.reserve(4096);
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setInterval(16);
    connect(
        refreshTimer_, &QTimer::timeout, this,
        [this]
        {
            if (!open_)
            {
                lastFrameTick_ = {};
                return;
            }
            const auto now = std::chrono::steady_clock::now();
            if (lastFrameTick_.time_since_epoch().count() != 0)
            {
                const double dtMs = std::chrono::duration<double, std::milli>(now - lastFrameTick_).count();
                frameMsSmoothed_ = frameMsSmoothed_ <= 0.0 ? dtMs : frameMsSmoothed_ + (dtMs - frameMsSmoothed_) * 0.1;
                if (accumulating_ && !complete_ && !isIdle_)
                {
                    frameStat_.Add(dtMs);
                    frameSamples_.push_back(static_cast<float>(dtMs));
                }
            }
            lastFrameTick_ = now;
            if (std::chrono::duration<double>(now - lastRefresh_).count() >= refreshInterval)
            {
                lastRefresh_ = now;
                RequestRefresh();
                sys_ = sampler_.Sample();
                if (accumulating_ && !complete_ && !isIdle_)
                {
                    cpuStat_.Add(sys_.cpuPercent);
                    memStat_.Add(static_cast<double>(sys_.memBytes));
                    if (sys_.gpuValid)
                    {
                        if (gpuStatScope_ != sys_.gpuScope)
                        {
                            gpuStat_.Reset();
                            gpuStatScope_ = sys_.gpuScope;
                        }
                        gpuStat_.Add(sys_.gpuPercent);
                    }
                }
            }
            Q_EMIT changed();
        }
    );
    // Timer is gated on open state (started in SetOpen), not running while closed.
}

void Benchmark::SetOpen(const bool open)
{
    if (open_ == open)
    {
        return;
    }
    open_ = open;
    if (refreshTimer_)
    {
        if (open_)
        {
            lastFrameTick_ = {};
            refreshTimer_->start();
        }
        else
        {
            refreshTimer_->stop();
            lastFrameTick_ = {};
        }
    }
    Q_EMIT changed();
}

void Benchmark::ApplySettings(bool limitDuration, float benchmarkDuration)
{
    limitDuration_ = limitDuration;
    benchmarkDuration_ = benchmarkDuration;
    if (auto* store = ctx_ ? ctx_->GetService<ISettingsStore>() : nullptr)
    {
        IModuleSettings& ps = store->GetModuleSettings(SettingsSection().c_str());
        SaveSettings(ps);
        ps.Save();
    }
    Q_EMIT changed();
}

namespace
{
// Frame time (ms) at the given percentile of `samples` — e.g. p=99 is the frame
// slower than 99% of frames, whose inverse is the "1% low" fps. Returns 0 when empty.
double PercentileMs(std::vector<float> samples, const double p)
{
    if (samples.empty())
    {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());
    const double rank = p / 100.0 * static_cast<double>(samples.size() - 1);
    const auto idx = static_cast<size_t>(std::clamp(rank, 0.0, static_cast<double>(samples.size() - 1)));
    return samples[idx];
}

QString Fps(const double ms)
{
    return ms > 0.0 ? QString::number(1000.0 / ms, 'f', 0) : QStringLiteral("0");
}

QString GpuLabel(const SysGpuScope scope)
{
    switch (scope)
    {
    case SysGpuScope::App:
        return QStringLiteral("GPU app");
    case SysGpuScope::Device:
        return QStringLiteral("GPU device");
    case SysGpuScope::Unavailable:
        break;
    }
    return QStringLiteral("GPU");
}

QVariantMap MetricRow(QString label, QString value, QString detail = {})
{
    QVariantMap row;
    row.insert(QStringLiteral("label"), std::move(label));
    row.insert(QStringLiteral("value"), std::move(value));
    row.insert(QStringLiteral("detail"), std::move(detail));
    return row;
}

QVariantMap MetricSection(QString title, QVariantList rows)
{
    QVariantMap section;
    section.insert(QStringLiteral("title"), std::move(title));
    section.insert(QStringLiteral("rows"), std::move(rows));
    return section;
}

QString AvgMaxDetail(const Stat& stat, const char* unit = "")
{
    if (stat.count == 0)
    {
        return {};
    }
    return QStringLiteral("avg %1%3 / max %2%3").arg(stat.Avg(), 0, 'f', 0).arg(stat.max, 0, 'f', 0).arg(unit);
}
} // namespace

QVariantList Benchmark::Sections() const
{
    const double liveFps = frameMsSmoothed_ > 0.0 ? 1000.0 / frameMsSmoothed_ : 0.0;
    QVariantList sections;

    QVariantList liveRows;
    liveRows << MetricRow(
        QStringLiteral("UI"), QStringLiteral("%1 fps").arg(liveFps, 0, 'f', 0),
        QStringLiteral("%1 ms").arg(frameMsSmoothed_, 0, 'f', 1)
    );
    liveRows << MetricRow(
        QStringLiteral("CPU app"), QStringLiteral("%1%").arg(sys_.cpuPercent, 0, 'f', 0), AvgMaxDetail(cpuStat_, "%")
    );
    const double toMiB = 1.0 / (1024.0 * 1024.0);
    liveRows << MetricRow(
        QStringLiteral("Mem app"), QStringLiteral("%1 MB").arg(static_cast<double>(sys_.memBytes) * toMiB, 0, 'f', 0),
        memStat_.count == 0 ? QString{}
                            : QStringLiteral("avg %1 MB / max %2 MB")
                                  .arg(memStat_.Avg() * toMiB, 0, 'f', 0)
                                  .arg(memStat_.max * toMiB, 0, 'f', 0)
    );
    if (sys_.gpuValid)
    {
        liveRows << MetricRow(
            GpuLabel(sys_.gpuScope), QStringLiteral("%1%").arg(sys_.gpuPercent, 0, 'f', 0), AvgMaxDetail(gpuStat_, "%")
        );
    }
    else
    {
        liveRows << MetricRow(QStringLiteral("GPU"), QStringLiteral("N/A"));
    }
    sections << MetricSection(QStringLiteral("Live"), liveRows);

    // ── Aggregated run results (only once samples exist) ─────────────────────────
    if (frameStat_.count > 0)
    {
        QVariantList runRows;
        // FPS min/max invert frame-time max/min: the slowest frame is the lowest fps.
        runRows << MetricRow(
            QStringLiteral("FPS"), QStringLiteral("avg %1").arg(Fps(frameStat_.Avg())),
            QStringLiteral("min %1 / max %2").arg(Fps(frameStat_.max), Fps(frameStat_.min))
        );
        runRows << MetricRow(
            QStringLiteral("Frame"), QStringLiteral("avg %1 ms").arg(frameStat_.Avg(), 0, 'f', 1),
            QStringLiteral("min %1 / max %2 / σ %3")
                .arg(frameStat_.min, 0, 'f', 1)
                .arg(frameStat_.max, 0, 'f', 1)
                .arg(frameStat_.Std(), 0, 'f', 2)
        );
        runRows << MetricRow(
            QStringLiteral("Lows"), QStringLiteral("1% %1 fps").arg(Fps(PercentileMs(frameSamples_, 99.0))),
            QStringLiteral("0.1% %1 fps").arg(Fps(PercentileMs(frameSamples_, 99.9)))
        );

        const double avgMs = frameStat_.Avg();
        const auto stutter = static_cast<int64_t>(std::count_if(
            frameSamples_.begin(), frameSamples_.end(),
            [avgMs](const float v)
            {
                return v > 2.0 * avgMs;
            }
        ));
        runRows << MetricRow(
            QStringLiteral("Stutter"), QString::number(stutter),
            QStringLiteral(">2x avg / frames %1 / %2 s").arg(frameStat_.count).arg(timePos_, 0, 'f', 0)
        );
        sections << MetricSection(QStringLiteral("Run"), runRows);
    }

    // ── Context: what produced these numbers ─────────────────────────────────────
    QVariantList contextRows;
    contextRows << MetricRow(QStringLiteral("Decoder"), QString::fromStdString(hwDec_));
    QString backend = gfxBackend_.empty() ? QStringLiteral("unknown") : QString::fromStdString(gfxBackend_);
    if (gpuIsNvidia_)
    {
        backend += QStringLiteral(" / NVIDIA");
    }
    contextRows << MetricRow(QStringLiteral("Backend"), backend);
    if (videoW_ > 0 && videoH_ > 0)
    {
        contextRows << MetricRow(QStringLiteral("Video"), QStringLiteral("%1x%2").arg(videoW_).arg(videoH_));
    }
    contextRows << MetricRow(
        QStringLiteral("Frames"), QStringLiteral("dropped %1").arg(dropped_),
        QStringLiteral("mistimed %1").arg(mistimed_)
    );
    contextRows << MetricRow(
        QStringLiteral("Decode"), QStringLiteral("errors %1").arg(decodeErrors_),
        QStringLiteral("cache misses %1").arg(cacheMisses_)
    );
    sections << MetricSection(QStringLiteral("Playback"), contextRows);

    return sections;
}

QString Benchmark::Summary() const
{
    QStringList rows;
    for (const QVariant& sectionValue : Sections())
    {
        const QVariantMap section = sectionValue.toMap();
        for (const QVariant& rowValue : section.value(QStringLiteral("rows")).toList())
        {
            const QVariantMap row = rowValue.toMap();
            const QString label = row.value(QStringLiteral("label")).toString();
            const QString value = row.value(QStringLiteral("value")).toString();
            const QString detail = row.value(QStringLiteral("detail")).toString();
            rows
                << (detail.isEmpty() ? QStringLiteral("%1  %2").arg(label, value)
                                     : QStringLiteral("%1  %2  (%3)").arg(label, value, detail));
        }
    }
    return rows.join('\n');
}

void Benchmark::chooseFile()
{
    if (auto* dialog = ctx_ ? ctx_->GetService<IFileDialog>() : nullptr)
    {
        dialog->OpenFile(
            [](const char* path, const bool ok, void* ud)
            {
                if (ok)
                {
                    static_cast<Benchmark*>(ud)->StartRun(path);
                    Q_EMIT static_cast<Benchmark*>(ud)->changed();
                }
            },
            this
        );
    }
}

void Benchmark::resetRun()
{
    accumulating_ = false;
    ResetStats();
    Q_EMIT changed();
}

void Benchmark::ResetStats()
{
    frameStat_.Reset();
    frameSamples_.clear();
    cpuStat_.Reset();
    memStat_.Reset();
    gpuStat_.Reset();
    gpuStatScope_ = SysGpuScope::Unavailable;
    decodeErrors_ = cacheMisses_ = 0;
    videoW_ = videoH_ = 0;
    complete_ = false;
}

void Benchmark::StartRun(const char* path)
{
    auto* player = ctx_ ? ctx_->GetService<IMediaPlayback>() : nullptr;
    if (!player || !path || !path[0])
    {
        return;
    }
    ResetStats();
    timePos_ = 0.0;
    accumulating_ = true;
    player->LoadFile(path, 0.0);
    player->SetPause(false);
}

void Benchmark::HandleMediaEvent(const MediaEvent& event)
{
    if (event.type != MediaEventType::PropertyChange)
    {
        return;
    }

    const auto& [prop, type, value] = event.property;
    if (prop == PlayerProperty::IdleActive && type == PropertyType::Flag)
    {
        isIdle_ = value.flag != 0;
        if (isIdle_)
        {
            hwDec_ = "N/A";
            dropped_ = mistimed_ = 0;
            decodeErrors_ = cacheMisses_ = 0;
            videoW_ = videoH_ = 0;
        }
        return;
    }

    if (prop == PlayerProperty::TimePos && type == PropertyType::Double)
    {
        timePos_ = value.dbl >= 0.0 ? value.dbl : 0.0;
        if (accumulating_ && !complete_ && limitDuration_ && timePos_ >= static_cast<double>(benchmarkDuration_))
        {
            if (auto* player = ctx_ ? ctx_->GetService<IMediaPlayback>() : nullptr)
            {
                player->SetPause(true);
            }
            complete_ = true;
            accumulating_ = false;
        }
    }
}

void Benchmark::RequestRefresh()
{
    auto* player = ctx_ ? ctx_->GetService<IMediaProperties>() : nullptr;
    if (!player)
    {
        return;
    }

    struct StrField
    {
        std::string* f;
        const char* def;
    };

    struct I64Field
    {
        int64_t* f;
        int64_t def;
    };

    static const auto strCb = [](const char* v, const bool ok, void* ud)
    {
        const auto* c = static_cast<StrField*>(ud);
        *c->f = (ok && v) ? v : c->def;
        delete c;
    };
    static const auto i64Cb = [](const int64_t v, const bool ok, void* ud)
    {
        const auto* c = static_cast<I64Field*>(ud);
        *c->f = ok ? v : c->def;
        delete c;
    };

    player->GetStringAsync(PlayerProperty::HwDecCurrent, strCb, new StrField{&hwDec_, "N/A"});
    player->GetInt64Async(PlayerProperty::DroppedFrames, i64Cb, new I64Field{&dropped_, 0});
    player->GetInt64Async(PlayerProperty::MistimedFrames, i64Cb, new I64Field{&mistimed_, 0});
    player->GetInt64Async(PlayerProperty::DecodeErrors, i64Cb, new I64Field{&decodeErrors_, 0});
    player->GetInt64Async(PlayerProperty::CacheMisses, i64Cb, new I64Field{&cacheMisses_, 0});
    player->GetDisplaySizeAsync(
        [](const DisplaySize* size, const bool ok, void* ud)
        {
            auto* self = static_cast<Benchmark*>(ud);
            self->videoW_ = (ok && size) ? size->width : 0;
            self->videoH_ = (ok && size) ? size->height : 0;
        },
        this
    );
}
