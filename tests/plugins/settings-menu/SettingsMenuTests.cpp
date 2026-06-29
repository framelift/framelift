#include "SettingsMenu.h"

#include "KeybindList.h"

#include "ModuleContext.h"
#include "Settings.h"
#include "TempIni.h"

#include "AudioSettings.h"
#include "CoreSettings.h"
#include "ThemeSettings.h"
#include "UISettings.h"

#include <QtCore/QVariantList>
#include <QtCore/QVariantMap>
#include <QtCore/Qt>
#include <cstring>
#include <gtest/gtest.h>
#include <ranges>

namespace
{
bool PagesContainId(const QVariantList& pages, const QString& id)
{
    return std::ranges::any_of(
        pages,
        [&](const QVariant& page)
        {
            return page.toMap().value(QStringLiteral("id")).toString() == id;
        }
    );
}
} // namespace

TEST(SettingsMenuTest, SeedsEditingModelFromContextOnInstall)
{
    Settings settings;
    settings.Get<UISettings>().panelWidth = 500.f; // non-default host values
    settings.Get<AudioSettings>().dynaudnormFrameLen = 321;
    settings.Get<FilesSettings>().videoExtensions = "avi;mov";

    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());

    SettingsMenu sm;
    sm.Install(ctx); // OnInstall seeds the model via the ABI-stable ctx getters

    EXPECT_FLOAT_EQ(sm.SettingFloat("ui.panelWidth"), 500.f);
    EXPECT_EQ(sm.SettingInt("audio.dynaudnormFrameLen"), 321);
    EXPECT_EQ(sm.SettingString("files.videoExtensions"), "avi;mov");
}

TEST(SettingsMenuTest, SeedsThemeFieldsFromContextOnInstall)
{
    Settings settings;
    settings.Get<ThemeSettings>().preset = "light";
    settings.Get<ThemeSettings>().accentColor = "#112233";

    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());

    SettingsMenu sm;
    sm.Install(ctx);

    EXPECT_EQ(sm.SettingString("theme.preset"), "light");
    EXPECT_EQ(sm.SettingString("theme.accentColor"), "#112233");
}

TEST(SettingsMenuTest, ExposesThemePageToQml)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());

    SettingsMenu sm;
    sm.Install(ctx);

    EXPECT_TRUE(PagesContainId(sm.QmlPages(), QStringLiteral("theme")));
}

TEST(SettingsMenuTest, DoesNotInventModulePagesFromRawFields)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());

    FrameLiftModuleSettingDesc desc{
        "fakePlugin.enabled",
        0,
        "",
        "1",
        [](void*) -> const char*
        {
            return "1";
        },
        [](void*, const char*)
        {
        },
        nullptr
    };
    ctx.Settings().RegisterModuleSetting(&desc);

    SettingsMenu sm;
    sm.Install(ctx);

    EXPECT_FALSE(PagesContainId(sm.QmlPages(), QStringLiteral("fakePlugin")));
}

TEST(SettingsMenuTest, OpenSetsVisible)
{
    SettingsMenu sm;
    EXPECT_FALSE(sm.IsOpen());
    sm.Open();
    EXPECT_TRUE(sm.IsOpen());
}

// Regression for #13: while the dialog is open it swallows key presses so global
// hotkeys don't fire underneath it.
TEST(SettingsMenuTest, SwallowsKeybindsWhileOpen)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());

    SettingsMenu sm;
    sm.Install(ctx);

    AppEvent key;
    key.type = AppEventType::KeyDown;
    key.AsKey() = {Keys::Space, Mod::None};

    // Closed: key passes through to the global hotkey layer.
    EXPECT_FALSE(sm.HandleKeyDownEvent(key));

    // Open: key swallowed.
    sm.Open();
    EXPECT_TRUE(sm.HandleKeyDownEvent(key));

    // Closed again: key passes through once more.
    sm.Close();
    EXPECT_FALSE(sm.IsOpen());
    EXPECT_FALSE(sm.HandleKeyDownEvent(key));
}

// ── Keybind list helpers (multi-key-per-action editing) ───────────────────────

TEST(KeybindListTest, SplitTrimsAndDropsEmpty)
{
    const auto keys = keybinds::Split(" Ctrl+F ; F2 ;");
    ASSERT_EQ(keys.size(), 2u);
    EXPECT_EQ(keys[0], "Ctrl+F");
    EXPECT_EQ(keys[1], "F2");
    EXPECT_TRUE(keybinds::Split("").empty());
}

TEST(KeybindListTest, ContainsMatchesWholeTokens)
{
    EXPECT_TRUE(keybinds::Contains("Ctrl+F;F2", "F2"));
    EXPECT_FALSE(keybinds::Contains("Ctrl+F;F2", "F"));
}

TEST(KeybindListTest, SlotReadsByPosition)
{
    EXPECT_EQ(keybinds::Slot("Ctrl+F;F2", 0), "Ctrl+F");
    EXPECT_EQ(keybinds::Slot("Ctrl+F;F2", 1), "F2");
    EXPECT_EQ(keybinds::Slot("Ctrl+F", 1), ""); // unset alternate
}

TEST(KeybindListTest, SetSlotEditsOneSlot)
{
    EXPECT_EQ(keybinds::SetSlot("", 0, "F"), "F");        // set primary
    EXPECT_EQ(keybinds::SetSlot("F", 1, "F2"), "F;F2");   // add alternate
    EXPECT_EQ(keybinds::SetSlot("F;F2", 0, ""), "F2");    // clear primary compacts
    EXPECT_EQ(keybinds::SetSlot("F;F2", 1, ""), "F");     // clear alternate
    EXPECT_EQ(keybinds::SetSlot("F;F2", 0, "G"), "G;F2"); // replace primary
}

TEST(KeybindListTest, SetSlotAvoidsDuplicateAcrossSlots)
{
    // Assigning a key already held by the other slot moves it rather than duplicating.
    EXPECT_EQ(keybinds::SetSlot("F;F2", 0, "F2"), "F2");
}

// ── Keybinds page capture (Qt-driven, into drafts) ────────────────────────────

namespace
{
QVariantMap FindKeybindEntry(const QVariantList& entries, const QString& action)
{
    for (const QVariant& v : entries)
    {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("action")).toString() == action)
        {
            return m;
        }
    }
    return {};
}
} // namespace

TEST(SettingsMenuKeybindsTest, CapturesCoreKeyIntoDraftSlots)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());
    SettingsMenu sm;
    sm.Install(ctx);

    // Primary slot: Ctrl+P (an unbound chord) replaces the default.
    sm.BeginCapture(QStringLiteral("togglePause"), 0);
    EXPECT_TRUE(sm.Capturing());
    sm.CaptureKey(static_cast<int>(Keys::P), static_cast<int>(Qt::ControlModifier));
    EXPECT_FALSE(sm.Capturing());
    EXPECT_EQ(sm.SettingString("keybinds.togglePause"), "Ctrl+P");

    // Alternate slot: add G.
    sm.BeginCapture(QStringLiteral("togglePause"), 1);
    sm.CaptureKey(static_cast<int>(Keys::G), 0);
    EXPECT_EQ(sm.SettingString("keybinds.togglePause"), "Ctrl+P;G");

    // Clearing the alternate compacts back to just the primary.
    sm.ClearKeybindSlot(QStringLiteral("togglePause"), 1);
    EXPECT_EQ(sm.SettingString("keybinds.togglePause"), "Ctrl+P");
}

TEST(SettingsMenuKeybindsTest, RejectsKeyAlreadyBoundElsewhere)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());
    SettingsMenu sm;
    sm.Install(ctx);

    // "M" is the default for toggleMute — capturing it for togglePause must conflict.
    sm.BeginCapture(QStringLiteral("togglePause"), 0);
    sm.CaptureKey(static_cast<int>(Keys::M), 0);

    EXPECT_FALSE(sm.Capturing());
    EXPECT_FALSE(sm.KeybindConflict().isEmpty());
    EXPECT_EQ(sm.SettingString("keybinds.togglePause"), "Space"); // unchanged default
}

TEST(SettingsMenuKeybindsTest, ExposesCoreAndPluginGroups)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());
    SettingsMenu sm;
    sm.Install(ctx);

    const QVariantList core = sm.CoreKeybindEntries();
    EXPECT_FALSE(FindKeybindEntry(core, QStringLiteral("togglePause")).isEmpty());

    // SettingsMenu's own "openSettings" keybind is auto-registered via ModuleBase and
    // must surface under its module's group.
    const QVariantList groups = sm.PluginKeybindGroups();
    ASSERT_FALSE(groups.isEmpty());
    bool foundOpenSettings = false;
    for (const QVariant& g : groups)
    {
        const QVariantList entries = g.toMap().value(QStringLiteral("entries")).toList();
        if (!FindKeybindEntry(entries, QStringLiteral("openSettings")).isEmpty())
        {
            foundOpenSettings = true;
            EXPECT_EQ(g.toMap().value(QStringLiteral("title")).toString(), QStringLiteral("SettingsMenu"));
        }
    }
    EXPECT_TRUE(foundOpenSettings);
}

TEST(SettingsMenuKeybindsTest, ResetRestoresPluginDefault)
{
    Settings settings;
    const TempFile ini;
    ModuleContext ctx("pref/", &settings, ini.str());
    SettingsMenu sm;
    sm.Install(ctx);

    // Rebind openSettings, then Reset should restore its factory default ("Ctrl+Comma").
    sm.BeginCapture(QStringLiteral("openSettings"), 0);
    sm.CaptureKey(static_cast<int>(Keys::F8), 0);

    auto openSettingsPrimary = [&]
    {
        const QVariantList groups = sm.PluginKeybindGroups();
        for (const QVariant& g : groups)
        {
            const QVariantMap e =
                FindKeybindEntry(g.toMap().value(QStringLiteral("entries")).toList(), QStringLiteral("openSettings"));
            if (!e.isEmpty())
            {
                return e.value(QStringLiteral("primary")).toString();
            }
        }
        return QString{};
    };
    EXPECT_EQ(openSettingsPrimary(), QStringLiteral("F8"));

    sm.resetActivePage();
    EXPECT_EQ(openSettingsPrimary(), QStringLiteral("Ctrl+Comma"));
}
