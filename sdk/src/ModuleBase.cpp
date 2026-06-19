#include <framelift/ContextHelpers.h>
#include <framelift/Guard.h>
#include <framelift/HotkeyHelpers.h>
#include <framelift/ModuleBase.h>
#include <framelift/ui/UIContext.h>
#include <cstring>

void ModuleBase::Install(IPluginContext& ctx) noexcept
{
    ctx_ = &ctx;
    // Note: ImGui context is now managed by the host-side UIContextImpl.
    // No SetImGuiContext call needed here.

    framelift::Guard(
        ModuleName(), "Install",
        [&]
        {
            // Cache the declarative tables once; the default hooks below consume them.
            fields_ = SettingsFields();
            keybinds_ = Keybinds();

            // Load module-specific settings; write defaults to disk on first run.
            IPluginSettings& ps = ctx.GetPluginSettings(SettingsSection().c_str());
            LoadSettings(ps);
            if (!ps.WasLoaded())
            {
                SaveSettings(ps);
                ps.Save();
            }

            // Load module keybinds from this module's own [<Module>.keybinds] section.
            IPluginSettings& kps = ctx.GetPluginSettings(KeybindsSection().c_str());
            LoadKeybinds(kps);
            const int keysBefore = kps.KeyCount();
            SaveKeybinds(kps);
            if (kps.KeyCount() > keysBefore)
            {
                kps.Save();
            }

            RegisterKeybinds(ctx);
            OnInstall(ctx);
        }
    );
}

void ModuleBase::BindHotkeys(Hotkeys& keys) noexcept
{
    framelift::Guard(
        ModuleName(), "BindHotkeys",
        [&]
        {
            OnBindHotkeys(keys);
        }
    );
}

void ModuleBase::Uninstall() noexcept
{
    framelift::Guard(
        ModuleName(), "Uninstall",
        [&]
        {
            OnUninstall();
        }
    );
}

void* ModuleBase::QueryInterface(const char* interfaceId) noexcept
{
    if (!interfaceId)
    {
        return nullptr;
    }
    if (std::strcmp(interfaceId, IHotkeyProvider::InterfaceId) == 0)
    {
        return static_cast<IHotkeyProvider*>(this);
    }
    if (std::strcmp(interfaceId, IEventHandler::InterfaceId) == 0)
    {
        return static_cast<IEventHandler*>(this);
    }
    if (std::strcmp(interfaceId, IMediaEventHandler::InterfaceId) == 0)
    {
        return static_cast<IMediaEventHandler*>(this);
    }
    if (std::strcmp(interfaceId, IShutdownHandler::InterfaceId) == 0)
    {
        return static_cast<IShutdownHandler*>(this);
    }
    return nullptr;
}

// Optional dispatch surfaces

bool ModuleBase::OnEvent(const AppEvent& e) noexcept
{
    return framelift::Guard(
        ModuleName(), "HandleEvent",
        [&]
        {
            return HandleEvent(e);
        }
    );
}

void ModuleBase::OnMediaEvent(const MediaEvent& e) noexcept
{
    framelift::Guard(
        ModuleName(), "HandleMediaEvent",
        [&]
        {
            HandleMediaEvent(e);
        }
    );
}

void ModuleBase::OnShutdown() noexcept
{
    framelift::Guard(
        ModuleName(), "HandleShutdown",
        [&]
        {
            HandleShutdown();
        }
    );
}

bool ModuleBase::HandleEvent(const AppEvent& e)
{
    // Route KeyDown to the key hook directly; OnEvent is already guarded.
    if (e.type == AppEventType::KeyDown)
    {
        return HandleKeyDownEvent(e);
    }
    return false;
}

// Table-driven hook defaults

void ModuleBase::LoadSettings(IPluginSettings& ps)
{
    framelift::LoadFields(ps, fields_);
}

void ModuleBase::SaveSettings(IPluginSettings& ps)
{
    framelift::SaveFields(ps, fields_);
}

void ModuleBase::LoadKeybinds(IPluginSettings& kps)
{
    for (const auto& kb : keybinds_)
    {
        *kb.storage = kps.GetString(kb.action, kb.def);
    }
}

void ModuleBase::SaveKeybinds(IPluginSettings& kps)
{
    for (const auto& kb : keybinds_)
    {
        kps.SetString(kb.action, kb.storage->c_str());
    }
}

void ModuleBase::RegisterKeybinds(IPluginContext& ctx)
{
    for (auto& kb : keybinds_)
    {
        framelift::RegisterKeybindEntry(ctx, kb.label, kb.action, *kb.storage);
    }
}

void ModuleBase::OnBindHotkeys(Hotkeys& keys)
{
    for (const auto& kb : keybinds_)
    {
        if (kb.onPress)
        {
            framelift::Bind(keys, kb.action, *kb.storage, kb.onPress);
        }
    }
}

void ModuleBase::SetupSettingsPage(IPluginContext& ctx, const bool visible)
{
    ctx.RegisterSettingsPage(
        ModuleName(),
        [](void* ud, UIContext& uiCtx)
        {
            framelift::Guard(
                "settings page render",
                [&]
                {
                    static_cast<ModuleBase*>(ud)->RenderSettings(uiCtx);
                }
            );
        },
        [](void* ud)
        {
            framelift::Guard(
                "settings page apply",
                [&]
                {
                    auto* fp = static_cast<ModuleBase*>(ud);
                    if (!fp->ctx_)
                    {
                        return;
                    }
                    IPluginSettings& ps = fp->ctx_->GetPluginSettings(fp->SettingsSection().c_str());
                    fp->SaveSettings(ps);
                    IPluginSettings& kps = fp->ctx_->GetPluginSettings(fp->KeybindsSection().c_str());
                    fp->SaveKeybinds(kps);
                }
            );
        },
        this, visible, nullptr
    );
}
