#pragma once

#include <QtCore/QObject>

class History;

class HistorySettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ Title CONSTANT)
    Q_PROPERTY(bool dirty READ Dirty NOTIFY changed)
    Q_PROPERTY(int maxEntries READ MaxEntries WRITE SetMaxEntries NOTIFY changed)

public:
    explicit HistorySettings(History& history);

    [[nodiscard]] QString Title() const;
    [[nodiscard]] bool Dirty() const;
    [[nodiscard]] int MaxEntries() const;

    void SetMaxEntries(int value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void reset();

Q_SIGNALS:
    void changed();

private:
    History& history_;
    bool dirty_ = false;
    int maxEntries_ = 200;
};
