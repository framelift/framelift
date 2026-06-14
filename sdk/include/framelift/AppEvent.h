#pragma once

#include <cstdint>

// ── Keyboard ──────────────────────────────────────────────────────────────────
// Key is an opaque integer whose value matches the platform keycode (SDL_Keycode
// in the SDL backend).  SdlAppWindow::TranslateKey() is therefore a trivial cast
// with no switch, so every key works automatically.
using Key = uint32_t;

// SDL3 keycodes for printable characters equal their Unicode codepoints.
// Scancode-based keys use SDLK_SCANCODE_MASK (0x40000000) | SDL_Scancode.
// These constants are reproduced here so callers never need <SDL3/SDL.h>.
namespace Keys
{
inline constexpr uint32_t ScancodeBase = 0x40000000u;

// ── Letters ───────────────────────────────────────────────────────────────
inline constexpr Key A = 'a', B = 'b', C = 'c', D = 'd', E = 'e', F = 'f', G = 'g', H = 'h', I = 'i', J = 'j', K = 'k',
                     L = 'l', M = 'm', N = 'n', O = 'o', P = 'p', Q = 'q', R = 'r', S = 's', T = 't', U = 'u', V = 'v',
                     W = 'w', X = 'x', Y = 'y', Z = 'z';

// ── Digits ─────────────────────────────────────────────────────────────────
inline constexpr Key Num0 = '0', Num1 = '1', Num2 = '2', Num3 = '3', Num4 = '4', Num5 = '5', Num6 = '6', Num7 = '7',
                     Num8 = '8', Num9 = '9';

// ── Punctuation ───────────────────────────────────────────────────────────────
inline constexpr Key Comma = ',';        // SDLK_COMMA
inline constexpr Key Period = '.';       // SDLK_PERIOD
inline constexpr Key Slash = '/';        // SDLK_SLASH
inline constexpr Key Backslash = '\\';   // SDLK_BACKSLASH
inline constexpr Key LeftBracket = '[';  // SDLK_LEFTBRACKET
inline constexpr Key RightBracket = ']'; // SDLK_RIGHTBRACKET
inline constexpr Key Minus = '-';        // SDLK_MINUS
inline constexpr Key Equals = '=';       // SDLK_EQUALS
inline constexpr Key Apostrophe = '\'';  // SDLK_APOSTROPHE
inline constexpr Key Backtick = '`';     // SDLK_GRAVE
inline constexpr Key Semicolon = ';';    // SDLK_SEMICOLON

// ── Special keys ──────────────────────────────────────────────────────────
inline constexpr Key Return = '\r';    // 0x0D
inline constexpr Key Space = ' ';      // 0x20
inline constexpr Key Tab = '\t';       // 0x09
inline constexpr Key Backspace = '\b'; // 0x08
inline constexpr Key Escape = 0x1Bu;

// ── Navigation (SDL_SCANCODE_* | ScancodeBase) ────────────────────────────
// SDL_SCANCODE_RIGHT=79, LEFT=80, DOWN=81, UP=82, HOME=74, END=77,
// PAGEUP=75, PAGEDOWN=78, INSERT=73, DELETE=76
inline constexpr Key Right = ScancodeBase | 79u;
inline constexpr Key Left = ScancodeBase | 80u;
inline constexpr Key Down = ScancodeBase | 81u;
inline constexpr Key Up = ScancodeBase | 82u;
inline constexpr Key Home = ScancodeBase | 74u;
inline constexpr Key End = ScancodeBase | 77u;
inline constexpr Key PageUp = ScancodeBase | 75u;
inline constexpr Key PageDown = ScancodeBase | 78u;
inline constexpr Key Insert = ScancodeBase | 73u;
inline constexpr Key Delete = ScancodeBase | 76u;

// ── Function keys (SDL_SCANCODE_F1=58 … F12=69) ───────────────────────────
inline constexpr Key F1 = ScancodeBase | 58u, F2 = ScancodeBase | 59u, F3 = ScancodeBase | 60u, F4 = ScancodeBase | 61u,
                     F5 = ScancodeBase | 62u, F6 = ScancodeBase | 63u, F7 = ScancodeBase | 64u, F8 = ScancodeBase | 65u,
                     F9 = ScancodeBase | 66u, F10 = ScancodeBase | 67u, F11 = ScancodeBase | 68u,
                     F12 = ScancodeBase | 69u;

// ── Lock keys ─────────────────────────────────────────────────────────────
inline constexpr Key CapsLock = ScancodeBase | 57u;    // SDL_SCANCODE_CAPSLOCK
inline constexpr Key PrintScreen = ScancodeBase | 70u; // SDL_SCANCODE_PRINTSCREEN
inline constexpr Key ScrollLock = ScancodeBase | 71u;  // SDL_SCANCODE_SCROLLLOCK
inline constexpr Key Pause = ScancodeBase | 72u;       // SDL_SCANCODE_PAUSE
inline constexpr Key NumLock = ScancodeBase | 83u;     // SDL_SCANCODE_NUMLOCKCLEAR

// ── Extended function keys (F13–F24) ──────────────────────────────────────
inline constexpr Key F13 = ScancodeBase | 104u, F14 = ScancodeBase | 105u, F15 = ScancodeBase | 106u,
                     F16 = ScancodeBase | 107u, F17 = ScancodeBase | 108u, F18 = ScancodeBase | 109u,
                     F19 = ScancodeBase | 110u, F20 = ScancodeBase | 111u, F21 = ScancodeBase | 112u,
                     F22 = ScancodeBase | 113u, F23 = ScancodeBase | 114u, F24 = ScancodeBase | 115u;

// ── Numpad (SDL_SCANCODE_KP_DIVIDE=84 … KP_PERIOD=99, KP_EQUALS=103) ─────
inline constexpr Key KeypadDivide = ScancodeBase | 84u;
inline constexpr Key KeypadMultiply = ScancodeBase | 85u;
inline constexpr Key KeypadMinus = ScancodeBase | 86u;
inline constexpr Key KeypadPlus = ScancodeBase | 87u;
inline constexpr Key KeypadEnter = ScancodeBase | 88u;
inline constexpr Key Keypad1 = ScancodeBase | 89u, Keypad2 = ScancodeBase | 90u, Keypad3 = ScancodeBase | 91u,
                     Keypad4 = ScancodeBase | 92u, Keypad5 = ScancodeBase | 93u, Keypad6 = ScancodeBase | 94u,
                     Keypad7 = ScancodeBase | 95u, Keypad8 = ScancodeBase | 96u, Keypad9 = ScancodeBase | 97u,
                     Keypad0 = ScancodeBase | 98u;
inline constexpr Key KeypadPeriod = ScancodeBase | 99u;
inline constexpr Key KeypadEquals = ScancodeBase | 103u;

// ── Application / context menu ────────────────────────────────────────────
inline constexpr Key Application = ScancodeBase | 101u; // SDL_SCANCODE_APPLICATION

// ── Editing helpers ───────────────────────────────────────────────────────
inline constexpr Key Help = ScancodeBase | 117u;   // SDL_SCANCODE_HELP
inline constexpr Key Menu = ScancodeBase | 118u;   // SDL_SCANCODE_MENU
inline constexpr Key Select = ScancodeBase | 119u; // SDL_SCANCODE_SELECT
inline constexpr Key Stop = ScancodeBase | 120u;   // SDL_SCANCODE_STOP
inline constexpr Key Again = ScancodeBase | 121u;  // SDL_SCANCODE_AGAIN (Redo)
inline constexpr Key Undo = ScancodeBase | 122u;   // SDL_SCANCODE_UNDO
inline constexpr Key Cut = ScancodeBase | 123u;    // SDL_SCANCODE_CUT
inline constexpr Key Copy = ScancodeBase | 124u;   // SDL_SCANCODE_COPY
inline constexpr Key Paste = ScancodeBase | 125u;  // SDL_SCANCODE_PASTE
inline constexpr Key Find = ScancodeBase | 126u;   // SDL_SCANCODE_FIND

// ── Volume ────────────────────────────────────────────────────────────────
inline constexpr Key Mute = ScancodeBase | 127u;       // SDL_SCANCODE_MUTE
inline constexpr Key VolumeUp = ScancodeBase | 128u;   // SDL_SCANCODE_VOLUMEUP
inline constexpr Key VolumeDown = ScancodeBase | 129u; // SDL_SCANCODE_VOLUMEDOWN

// ── Media transport (SDL_SCANCODE_MEDIA_*) ────────────────────────────────
inline constexpr Key MediaPlay = ScancodeBase | 262u;
inline constexpr Key MediaPause = ScancodeBase | 263u;
inline constexpr Key MediaRecord = ScancodeBase | 264u;
inline constexpr Key MediaFastForward = ScancodeBase | 265u;
inline constexpr Key MediaRewind = ScancodeBase | 266u;
inline constexpr Key MediaNext = ScancodeBase | 267u;
inline constexpr Key MediaPrev = ScancodeBase | 268u;
inline constexpr Key MediaStop = ScancodeBase | 269u;
inline constexpr Key MediaEject = ScancodeBase | 270u;
inline constexpr Key MediaPlayPause = ScancodeBase | 271u;
inline constexpr Key MediaSelect = ScancodeBase | 272u;

// ── Application control (SDL_SCANCODE_AC_*) ───────────────────────────────
inline constexpr Key AcNew = ScancodeBase | 273u;
inline constexpr Key AcOpen = ScancodeBase | 274u;
inline constexpr Key AcClose = ScancodeBase | 275u;
inline constexpr Key AcExit = ScancodeBase | 276u;
inline constexpr Key AcSave = ScancodeBase | 277u;
inline constexpr Key AcPrint = ScancodeBase | 278u;
inline constexpr Key AcProperties = ScancodeBase | 279u;
inline constexpr Key AcSearch = ScancodeBase | 280u;
inline constexpr Key AcHome = ScancodeBase | 281u;
inline constexpr Key AcBack = ScancodeBase | 282u;
inline constexpr Key AcForward = ScancodeBase | 283u;
inline constexpr Key AcStop = ScancodeBase | 284u;
inline constexpr Key AcRefresh = ScancodeBase | 285u;
inline constexpr Key AcBookmarks = ScancodeBase | 286u;

inline constexpr Key Unknown = 0u;
} // namespace Keys

// Bitfield modifier flags (OR-able)
enum class Mod : uint32_t
{
    None = 0,
    Ctrl = 1u << 0,
    Shift = 1u << 1,
    Alt = 1u << 2,
};

[[nodiscard]] inline Mod operator|(Mod a, Mod b)
{
    return static_cast<Mod>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

[[nodiscard]] inline Mod operator&(Mod a, Mod b)
{
    return static_cast<Mod>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// Returns true if flag is set in the modifier state (e.g. ModSet(e.mods, Mod::Ctrl)).
[[nodiscard]] inline bool ModSet(const Mod state, const Mod flag)
{
    return (state & flag) != Mod::None;
}

// ── File filters ──────────────────────────────────────────────────────────────

struct FileFilter
{
    const char* name;    // e.g. "Video files"
    const char* pattern; // e.g. "mp4;mkv;avi"
};

// ── Application events ────────────────────────────────────────────────────────

enum class AppEventType : std::uint8_t
{
    None,            // no event / timeout
    Quit,            // application quit requested
    WindowExposed,   // window content needs repaint
    KeyDown,         // keyboard key pressed
    KeyUp,           // keyboard key released
    RenderUpdate,    // player has a new video frame ready
    PlayerWakeup,    // player has events to drain
    MouseButtonDown, // any mouse button pressed
    MouseMotion,     // mouse cursor moved
    DropFile,        // a file was dragged and dropped onto the window
    MouseWheel,      // mouse wheel scrolled
    Custom,          // any other platform event
};

// An application-level event translated from a raw platform event.
//
// The `type` field identifies which union member is active:
//
//   type                  active member
//   ──────────────────────────────────────────────────────────────
//   None                  — (no valid member)
//   Quit                  — (no valid member)
//   WindowExposed         — (no valid member)
//   RenderUpdate          — (no valid member)
//   PlayerWakeup          — (no valid member)
//   MouseButtonDown       — (no valid member)
//   MouseMotion           — (no valid member)
//   MouseWheel            — (no valid member; scroll deltas live in nativeStorage)
//   KeyDown               key
//   KeyUp                 key
//   DropFile              file
//   Custom                custom
//
// All payload types are POD — no heap allocation, safe across DLL boundaries.
struct AppEvent
{
    AppEventType type = AppEventType::None;

    // Opaque storage for the backing native event.
    // SDL_Event is guaranteed to be exactly 128 bytes — fits any SDL3 event union.
    // SdlAppWindow::ImGuiProcessEvent() reads this field without exposing SDL types.
    alignas(8) uint8_t nativeStorage[128]{};

    // Payload for KeyDown / KeyUp events.
    struct KeyPayload
    {
        Key key = Keys::Unknown;
        Mod mods = Mod::None;
    };

    // Payload for DropFile events.
    // filePath is a host-owned pointer valid only for the duration of the OnEvent() call.
    // nullptr when the platform provided no path (malformed drop).
    struct FilePayload
    {
        const char* filePath = nullptr;
    };

    // Payload for Custom events — any platform event not handled by the cases above.
    //
    // eventType is the raw SDL event type (e.g. SDL_EVENT_USER + n for app-registered
    // events, or an unrecognized SDL system type).  Consumers compare it against their
    // own registered type ID to decide whether the event belongs to them.
    //
    // userData1 is meaningful only for SDL_EVENT_USER and above (i.e. app-registered
    // events).  Each subsystem that pushes a custom event owns its own type ID and is
    // responsible for defining what userData1 points to.  The receiver must cast it to
    // the matching type and take ownership if it carries heap-allocated data:
    //
    //   FileDialogServiceImpl::HandleEvent — casts to its Payload* and wraps in unique_ptr
    //   Playlist                           — only matches on eventType; userData1 is unused
    //
    // For non-USER SDL events userData1 is always nullptr.
    struct CustomPayload
    {
        uint32_t eventType = 0;
        void* userData1 = nullptr;
    };

    union {
        KeyPayload key{};
        FilePayload file;
        CustomPayload custom;
    };

    [[nodiscard]] const KeyPayload& AsKey() const noexcept
    {
        return key;
    }

    [[nodiscard]] KeyPayload& AsKey() noexcept
    {
        return key;
    }

    [[nodiscard]] const FilePayload& AsFile() const noexcept
    {
        return file;
    }

    [[nodiscard]] FilePayload& AsFile() noexcept
    {
        return file;
    }

    [[nodiscard]] const CustomPayload& AsCustom() const noexcept
    {
        return custom;
    }

    [[nodiscard]] CustomPayload& AsCustom() noexcept
    {
        return custom;
    }
};