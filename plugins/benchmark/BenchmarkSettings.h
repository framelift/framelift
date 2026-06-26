#pragma once

#include <QtCore/QObject>

class Benchmark;

class BenchmarkSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ Title CONSTANT)
    Q_PROPERTY(bool dirty READ Dirty NOTIFY changed)
    Q_PROPERTY(bool limitDuration READ LimitDuration WRITE SetLimitDuration NOTIFY changed)
    Q_PROPERTY(float benchmarkDuration READ BenchmarkDuration WRITE SetBenchmarkDuration NOTIFY changed)

public:
    explicit BenchmarkSettings(Benchmark& benchmark);

    [[nodiscard]] QString Title() const;
    [[nodiscard]] bool Dirty() const;
    [[nodiscard]] bool LimitDuration() const;
    [[nodiscard]] float BenchmarkDuration() const;

    void SetLimitDuration(bool value);
    void SetBenchmarkDuration(float value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void reset();

Q_SIGNALS:
    void changed();

private:
    void MarkDirty();

    Benchmark& benchmark_;
    bool dirty_ = false;
    bool limitDuration_ = false;
    float benchmarkDuration_ = 30.0f;
};
