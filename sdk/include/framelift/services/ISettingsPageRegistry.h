#pragma once

class QObject;

struct FrameLiftSettingsPageDesc
{
    const char* id;
    const char* title;
    const char* qmlUrl;
    QObject* viewModel;
    int order;
};

class ISettingsPageRegistry
{
public:
    static constexpr const char* InterfaceId = "framelift.ISettingsPageRegistry";
    virtual ~ISettingsPageRegistry() = default;

    virtual void RegisterSettingsPage(
        const char* id, const char* title, const char* qmlUrl, QObject* viewModel, int order
    ) noexcept = 0;

    virtual void EnumerateSettingsPages(
        void (*visit)(const FrameLiftSettingsPageDesc* desc, void* visitUd), void* visitUd
    ) const noexcept = 0;
};
