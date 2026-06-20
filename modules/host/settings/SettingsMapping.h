#pragma once

// Umbrella header. The pure Settings→player-option mappers now live in their
// owning modules; this header just re-exports them so host call sites keep one
// include. Each mapper takes its module's settings sub-struct.
//
//   ffmpeg     : ParamsFromSettings, PlaybackOptsFromSettings, VideoDecodeModeFromSettings,
//                AudioPrefsFromSettings, SubtitleStyleFromSettings
//   read-ahead : ReadAheadOptsFromSettings

#include "CacheSettings.h"          // host/read-ahead  (ReadAheadOptsFromSettings)
#include "FFmpegSettingsMapping.h"  // media/ffmpeg
