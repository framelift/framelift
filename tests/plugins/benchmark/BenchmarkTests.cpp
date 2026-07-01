#include "Benchmark.h"
#include "SysStats.h"

#include "QtTestRunner.h"

#include <QtCore/QVariantMap>
#include <QtTest/QtTest>

class BenchmarkTest final : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void TogglesVisibility()
    {
        Benchmark b;
        QVERIFY(!(b.IsOpen())); // closed by default
        b.Toggle();
        QVERIFY(b.IsOpen());
        b.Toggle();
        QVERIFY(!(b.IsOpen()));
    }

    void MediaEventsDoNotCrashWhileClosed()
    {
        Benchmark b;
        MediaEvent e;
        e.type = MediaEventType::PropertyChange;
        e.property.prop = PlayerProperty::IdleActive;
        e.property.type = PropertyType::Flag;
        e.property.value.flag = 1;
        b.OnMediaEvent(e); // must be safe with no player/context
    }

    void SamplerReportsNormalizedValues()
    {
        SysSampler sampler;
        const SysSample first = sampler.Sample(); // primes the rate counters
        const SysSample second = sampler.Sample();

        // CPU is normalized across cores and clamped; memory is non-negative.
        QVERIFY((second.cpuPercent) >= (0.0));
        QVERIFY((second.cpuPercent) <= (100.0));
        QVERIFY((first.memBytes) >= (0u));

        // GPU is best-effort: when reported it must be a sane percentage.
        if (second.gpuValid)
        {
            QVERIFY((second.gpuPercent) >= (0.0));
            QVERIFY((second.gpuPercent) <= (100.0));
        }
    }

    void SummaryLabelsAppScopedStats()
    {
        Benchmark b;
        SysSample sample;
        sample.cpuPercent = 12.0;
        sample.memBytes = 64ull * 1024ull * 1024ull;
        sample.gpuPercent = 34.0;
        sample.gpuValid = true;
        sample.gpuScope = SysGpuScope::App;
        b.SetSysSampleForTest(sample);
        b.AddGpuStatForTest(34.0, SysGpuScope::App);

        const QString summary = b.Summary();
        QVERIFY(summary.contains(QStringLiteral("CPU app")));
        QVERIFY(summary.contains(QStringLiteral("Mem app")));
        QVERIFY(summary.contains(QStringLiteral("GPU app")));
        QVERIFY(!(summary.contains(QStringLiteral("GPU device"))));
    }

    void SummaryLabelsDeviceGpuFallback()
    {
        Benchmark b;
        SysSample sample;
        sample.gpuPercent = 55.0;
        sample.gpuValid = true;
        sample.gpuScope = SysGpuScope::Device;
        b.SetSysSampleForTest(sample);
        b.AddGpuStatForTest(55.0, SysGpuScope::Device);

        const QString summary = b.Summary();
        QVERIFY(summary.contains(QStringLiteral("GPU device")));
        QVERIFY(!(summary.contains(QStringLiteral("GPU app"))));
    }

    void SummaryLabelsUnavailableGpu()
    {
        Benchmark b;
        SysSample sample;
        sample.gpuValid = false;
        sample.gpuScope = SysGpuScope::Unavailable;
        b.SetSysSampleForTest(sample);

        const QString summary = b.Summary();
        QVERIFY(summary.contains(QStringLiteral("GPU  N/A")));
    }

    void ExposesStructuredSections()
    {
        Benchmark b;
        SysSample sample;
        sample.cpuPercent = 12.0;
        sample.memBytes = 64ull * 1024ull * 1024ull;
        sample.gpuPercent = 34.0;
        sample.gpuValid = true;
        sample.gpuScope = SysGpuScope::App;
        b.SetSysSampleForTest(sample);

        const QVariantList sections = b.Sections();
        QVERIFY(!(sections.empty()));
        const QVariantMap live = sections.front().toMap();
        QCOMPARE(live.value(QStringLiteral("title")).toString(), QStringLiteral("Live"));

        const QVariantList rows = live.value(QStringLiteral("rows")).toList();
        QVERIFY((rows.size()) >= (4));
        QCOMPARE(rows.at(1).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("CPU app"));
        QCOMPARE(rows.at(3).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("GPU app"));
    }

    void HidesLiveAggregateDetailsBeforeRunSamples()
    {
        Benchmark b;
        SysSample sample;
        sample.cpuPercent = 12.0;
        sample.memBytes = 64ull * 1024ull * 1024ull;
        sample.gpuPercent = 34.0;
        sample.gpuValid = true;
        sample.gpuScope = SysGpuScope::App;
        b.SetSysSampleForTest(sample);

        const QVariantMap live = b.Sections().front().toMap();
        const QVariantList rows = live.value(QStringLiteral("rows")).toList();
        QCOMPARE(rows.at(1).toMap().value(QStringLiteral("detail")).toString(), QString{});
        QCOMPARE(rows.at(2).toMap().value(QStringLiteral("detail")).toString(), QString{});
        QCOMPARE(rows.at(3).toMap().value(QStringLiteral("detail")).toString(), QString{});

        const QString summary = b.Summary();
        QVERIFY(!(summary.contains(QStringLiteral("avg 0"))));
        QVERIFY(!(summary.contains(QStringLiteral("max 0"))));
    }

#if !defined(_WIN32)
    void ParsesDrmFdinfoCounters()
    {
        const auto counters = ParseDrmFdinfoCounters(
            "pos:\t0\n"
            "flags:\t02100002\n"
            "drm-driver:\tamdgpu\n"
            "drm-pdev:\t0000:03:00.0\n"
            "drm-client-id:\t12\n"
            "drm-engine-gfx:\t1000 ns\n"
            "drm-engine-compute:\t250 ns\n",
            "fd42"
        );

        QVERIFY(counters.valid);
        QCOMPARE(QString::fromStdString(counters.clientKey), QStringLiteral("amdgpu|0000:03:00.0|12"));
        QCOMPARE(static_cast<qulonglong>(counters.engineNs), static_cast<qulonglong>(1250));
    }

    void DeduplicatesDrmFdinfoClients()
    {
        std::vector<DrmFdinfoCounters> samples;
        samples.push_back(ParseDrmFdinfoCounters(
            "drm-driver:\tamdgpu\n"
            "drm-pdev:\t0000:03:00.0\n"
            "drm-client-id:\t12\n"
            "drm-engine-gfx:\t1000 ns\n",
            "fd1"
        ));
        samples.push_back(ParseDrmFdinfoCounters(
            "drm-driver:\tamdgpu\n"
            "drm-pdev:\t0000:03:00.0\n"
            "drm-client-id:\t12\n"
            "drm-engine-gfx:\t1200 ns\n",
            "fd2"
        ));
        samples.push_back(ParseDrmFdinfoCounters("drm-engine-gfx:\t50 ns\n", "fd3"));

        QCOMPARE(static_cast<qulonglong>(TotalUniqueDrmEngineNs(samples)), static_cast<qulonglong>(1250));
    }
#endif
};

namespace
{
const ::framelift::test::Registrar<BenchmarkTest> kRegisterBenchmarkTest{"BenchmarkTest"};
}

#include "BenchmarkTests.moc"
