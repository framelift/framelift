# FrameLift

A lightweight video player built with Dear ImGui, SDL3, FFmpeg, and libass.

## Features

- **Video & image playback** — MP4, MKV, AVI, MOV, WMV, FLV, WebM, M4V, MPG, MPEG, PNG, JPG, GIF, BMP, WebP, and
  anything FFmpeg supports
- **Playlist** — automatically populated from the opened file's directory, with optional recursive subdirectory
  scanning, shuffle, and auto-reload on directory changes
- **History with resume** — recently played files with saved positions, searchable side panel
- **Multi-track audio & subtitles** — switch tracks and adjust subtitle delay at runtime
- **Dynamic audio normalization** — optional dynaudnorm filter with tunable parameters
- **Image slideshow** — configurable auto-advance interval for image-only or mixed playlists
- **Fully rebindable hotkeys** — every action has a default keybind, all overridable in settings
- **Auto-update** — background update check against GitHub Releases; applies on next launch
- **Debug overlay** — live playback stats (Tab to toggle)

## Installation

Download the latest release archive from the [Releases](../../releases) page, extract, and run `FrameLift`. No installer
needed — the archive is self-contained.

**Platforms:** Windows (x86_64) and Linux (X11/Wayland via SDL3). macOS is not yet supported.

**Requirements:** OpenGL 3.0+ GPU. On Linux, SDL3, FFmpeg, and libass from your distribution (e.g. `libsdl3-0`,
`libavcodec`, `libavformat`, `libavfilter`, `libass9`).

## Plugin SDK

Every FrameLift capability ships as a runtime-loaded plugin DLL. You can build your own against
the **dependency-free plugin SDK** (`framelift-sdk-<ver>.zip`, published as a Release
asset on every version tag). The host↔plugin boundary is a COM-like binary ABI,
so a plugin built with any compatible Windows compiler loads regardless of how
the host was built. See the SDK's bundled `README.md` (and [sdk/](sdk/) in this
repo) for the API and a worked example.

## Configuration

Settings are stored in the platform config directory and are edited via the in-app Settings menu (Ctrl+,). Key options:

| Setting                  | Default      | Description                                     |
|--------------------------|--------------|-------------------------------------------------|
| Scan subdirectories      | on           | Recursively scan subdirs when building playlist |
| Max scan depth           | 5            | Subdirectory recursion limit                    |
| Mixed playlists          | on           | Allow videos and images in the same playlist    |
| Image slideshow duration | 5 s          | Auto-advance interval for images                |
| Auto-reload playlist     | on           | Watch directory for file changes                |
| Max history entries      | 200          | How many files to keep in history               |
| Dynamic normalization    | configurable | Frame length, smoothing, peak, max gain         |

Video and image file extensions are fully customizable (semicolon-separated lists).

## Repository Layout

```
FrameLift/
├── sdk/                    # Public plugin SDK — everything a plugin author needs
│   ├── include/framelift/       # Public headers (include path: sdk/include)
│   │   ├── core.h          # Umbrella: plugin lifecycle, context, ABI, events, hotkeys
│   │   ├── ui.h            # Umbrella: IRenderable, Panel, UIContext, widgets
│   │   ├── services.h      # Umbrella: cross-plugin service interfaces
│   │   ├── platform.h      # Umbrella: IMediaPlayer, IAppWindow, IDirWatcher
│   │   ├── services/       # Per-plugin service interfaces (IOverlay, IHistory, …)
│   │   ├── platform/       # Platform service interfaces
│   │   └── ui/             # UI helper headers (Panel, UIContext, Widgets, …)
│   └── src/                # SDK helper sources compiled into each plugin
│
├── src/                    # Host application (FrameLift.exe) — not shipped to plugin authors
│   ├── platform/           # Platform implementations (SDL3, FFmpeg, libass, Win32/Linux dir watchers)
│   └── ui/                 # Host-only UI (Dialog)
│
├── plugins/                # Plugins (each compiled to a separate DLL)
│   ├── Overlay/
│   ├── Playlist/
│   ├── History/
│   ├── SettingsMenu/
│   ├── DebugOverlay/
│   └── Updater/
│
├── vcpkg.json               # Windows native libs (SDL3, FFmpeg, libass) — vcpkg manifest
└── CMakeLists.txt
```

Plugins include only `<framelift/core.h>`, `<framelift/ui.h>`, `<framelift/services.h>`, and/or `<framelift/platform.h>` — they never
include host internals. The host (`src/`) has no compile-time knowledge of specific plugins; all plugins are loaded at
runtime from the `Plugins/` directory next to the executable.

## Building from Source

### Prerequisites

- CMake 4.3.0+
- C++23-capable compiler (GCC, Clang, or MSVC)
- `clang-format` on PATH (for the pre-commit hook)
- **Linux only:** development packages for SDL3, FFmpeg, libass, and OpenGL
  (e.g. on Ubuntu 25.04+: `sudo apt install libsdl3-dev libavformat-dev libavcodec-dev libavutil-dev libswscale-dev
  libswresample-dev libavfilter-dev libass-dev libgl1-mesa-dev pkg-config`).
- **Windows only:** [vcpkg](https://vcpkg.io) (`VCPKG_ROOT` set) — SDL3, FFmpeg, and libass are all resolved from
  the manifest in `vcpkg.json`. Configure with the vcpkg preset (`CMakePresets.json`).

### Steps

```sh
git clone <repo-url>
cd FrameLift

# Activate formatting hook (one-time per clone)
git config core.hooksPath .github/hooks

# Configure and build
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --config Debug
```

Output: `cmake-build-debug/FrameLift` (`.exe` on Windows). On Windows the required shared libraries are copied next to the
executable; on Linux SDL3/FFmpeg/libass are resolved from the system. Plugins are placed in `cmake-build-debug/Plugins/`
(`.dll` on Windows, `.so` on Linux). The auto-updater is Windows-only and is disabled automatically elsewhere.

### Dependencies (fetched by CMake)

| Library       | Version |
|---------------|---------|
| Dear ImGui    | 1.91.9b |
| SDL3          | 3.4.8   |
| FFmpeg        | vcpkg / system |
| libass        | vcpkg / system |
| spdlog        | 1.17.0  |
| nlohmann/json | 3.11.3  |
| stb           | latest  |

## License

[zlib](LICENSE)
