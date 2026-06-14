#include <framelift/ContextHelpers.h>
#include <framelift/Guard.h>
#include <framelift/HotkeyHelpers.h>
#include <framelift/PluginBase.h>
#include <framelift/ui/UIContext.h>

void PluginBase::Install(IPluginContext& ctx) noexcept
{
    ctx_ = &ctx;
    // Note: ImGui context is now managed by the host-side UIContextImpl.
    // No SetImGuiContext call needed here.

    framelift::Guard(
        PluginName(), "Install",
        [&]
        {
            // Cache the declarative tables once; the default hooks below consume them.
            fields_ = SettingsFields();
            keybinds_ = Keybinds();

            // Load plugin-specific settings; write defaults to disk on first run.
            IPluginSettings& ps = ctx.GetPluginSettings(SettingsSection().c_str());
            LoadSettings(ps);
            if (!ps.WasLoaded())
            {
                SaveSettings(ps);
                ps.Save();
            }

            // Load plugin keybinds from this plugin's own [<Plugin>.keybinds] section.
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

void PluginBase::BindHotkeys(Hotkeys& keys) noexcept
{
    framelift::Guard(
        PluginName(), "BindHotkeys",
        [&]
        {
            OnBindHotkeys(keys);
        }
    );
}

// ── Sealed IPlugin event hooks ────────────────────────────────────────────────

bool PluginBase::OnEvent(const AppEvent& e) noexcept
{
    return framelift::Guard(
        PluginName(), "HandleEvent",
        [&]
        {
            return HandleEvent(e);
        }
    );
}

bool PluginBase::OnKeyDownEvent(const AppEvent& e) noexcept
{
    return framelift::Guard(
        PluginName(), "HandleKeyDownEvent",
        [&]
        {
            return HandleKeyDownEvent(e);
        }
    );
}

void PluginBase::OnMediaEvent(const MediaEvent& e) noexcept
{
    framelift::Guard(
        PluginName(), "HandleMediaEvent",
        [&]
        {
            HandleMediaEvent(e);
        }
    );
}

void PluginBase::OnShutdown() noexcept
{
    framelift::Guard(
        PluginName(), "HandleShutdown",
        [&]
        {
            HandleShutdown();
        }
    );
}

bool PluginBase::HandleEvent(const AppEvent& e)
{
    // Mirrors IPlugin::OnEvent's default: route KeyDown to the key hook
    // (directly, not via the sealed OnKeyDownEvent — it is already guarded).
    if (e.type == AppEventType::KeyDown)
    {
        return HandleKeyDownEvent(e);
    }
    return false;
}

// ── Table-driven hook defaults ────────────────────────────────────────────────

void PluginBase::LoadSettings(IPluginSettings& ps)
{
    framelift::LoadFields(ps, fields_);
}

void PluginBase::SaveSettings(IPluginSettings& ps)
{
    framelift::SaveFields(ps, fields_);
}

void PluginBase::LoadKeybinds(IPluginSettings& kps)
{
    for (const auto& kb : keybinds_)
    {
        *kb.storage = kps.GetString(kb.action, kb.def);
    }
}

void PluginBase::SaveKeybinds(IPluginSettings& kps)
{
    for (const auto& kb : keybinds_)
    {
        kps.SetString(kb.action, kb.storage->c_str());
    }
}

void PluginBase::RegisterKeybinds(IPluginContext& ctx)
{
    for (auto& kb : keybinds_)
    {
        framelift::RegisterKeybindEntry(ctx, kb.label, kb.action, *kb.storage);
    }
}

void PluginBase::OnBindHotkeys(Hotkeys& keys)
{
    for (const auto& kb : keybinds_)
    {
        if (kb.onPress)
        {
            framelift::Bind(keys, kb.action, *kb.storage, kb.onPress);
        }
    }
}

void PluginBase::SetupSettingsPage(IPluginContext& ctx, const bool visible)
{
    ctx.RegisterSettingsPage(
        PluginName(),
        [](void* ud, UIContext& uiCtx)
        {
            framelift::Guard(
                "settings page render",
                [&]
                {
                    static_cast<PluginBase*>(ud)->RenderSettings(uiCtx);
                }
            );
        },
        [](void* ud)
        {
            framelift::Guard(
                "settings page apply",
                [&]
                {
                    auto* fp = static_cast<PluginBase*>(ud);
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