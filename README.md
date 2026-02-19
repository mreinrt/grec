# grec

`grec` is a standalone GTK3/X11-based screen and audio recorder for Linux with a **separate preferences application**. It allows you to capture either your **full screen** or a **selected region** and can record audio from your microphone, system output, or both simultaneously. Output can be saved in **MP4, MKV, or GIF format**.

It is designed for desktop environments like XFCE, providing a quick and lightweight way to record your screen with extensive configuration options.

* * *

## Features

### Core Recording
  * Full-screen or region-based recording
  * Transparent overlay for region selection with **customizable color and transparency**
  * Red blinking indicator for active recording (now stays out of your taskbar - no focus stealing!)
  * Hotkey control (Ctrl + Shift + End) to stop recording
  * X/close button stops recording automatically

### New in v2.1 - Facebook Compatibility & UI Improvements
  * **Facebook-friendly MP4**: Now uses H.264 (libx264) codec instead of mpeg4 for maximum platform compatibility
  * **Smart scaling**: Automatically adjusts region dimensions to even numbers (required for H.264)
  * **Optimized encoding**: Uses `-preset fast` and `-crf 23` for perfect balance of quality and performance
  * **No taskbar flashing**: Recording indicator now uses popup window and doesn't steal focus

### New in v2.0 - Standalone Preferences App (`grec_prefs`)
  * **Separate GUI preferences application** that can be launched independently
  * Configuration saved to `~/.config/grec.conf` - shared between both apps
  * **Format selection**: MP4 (most compatible), MKV (feature rich), or GIF (animated, no audio)
  * **Video quality presets**: High (largest files), Balanced, Small (smallest files)
  * **Framerate control**: 15, 24, 30, or 60 fps
  * **Audio source selection**: Choose microphone, system audio, or **both mixed together**
  * **Audio quality**: High (192kbps), Medium (128kbps), or Low (64kbps)
  * **Region selector color**: Customize the overlay color and transparency with live preview
  * **Post-recording options**: Toggle save dialog and desktop notifications

### Audio Intelligence
  * **Auto-detects PulseAudio sources** using `pactl` - works on any distribution
  * Falls back to ALSA default if PulseAudio is not available
  * No hardcoded device names - detects your actual microphone and system audio monitor
  * Mixes multiple audio streams when both sources are selected

* * *

## Requirements

  * Linux with GTK3 and X11 (tested on Gentoo/XFCE4, should work on other distros)
  * GTK3 development libraries
  * X11 development libraries
  * FFmpeg installed and available in PATH (with **x264 support** for H.264 encoding)
  * GCC (or compatible C++ compiler)
  * **Optional but recommended**: `pulseaudio-utils` (provides `pactl` for audio source detection)

### Gentoo example:
```
sudo emerge media-video/ffmpeg dev-libs/gtk+:3.0 x11-libs/libX11 media-sound/pulseaudio
# Make sure x264 USE flag is enabled for ffmpeg:
echo "media-video/ffmpeg x264" | sudo tee -a /etc/portage/package.use/ffmpeg
sudo emerge --ask --newuse media-video/ffmpeg
```

### Ubuntu/Debian example:
```
sudo apt install ffmpeg libgtk-3-dev libx11-dev pulseaudio-utils
```

* * *

## Building

Clone or download the repository:
git clone https://github.com/mreinrt/grec.git
cd grec

Compile both programs using the Makefile:
make

Or compile individually:
make grec        # Build only the recorder
make grec_prefs  # Build only the preferences app

* * *

## Installation

(Optional) Move the binaries to a directory in your `$PATH`:
sudo make install

Or manually:
sudo cp grec grec_prefs /usr/local/bin/
sudo chmod +x /usr/local/bin/grec /usr/local/bin/grec_prefs

* * *

## Usage

### 1. Configure your preferences first
Run the standalone preferences application:
./grec_prefs

This will create `~/.config/grec.conf` with your settings. The preferences dialog includes tabs for:
  * **Output**: Choose format, save directory, and filename pattern
  * **Video**: Select quality preset and framerate
  * **Audio**: Choose microphone, system audio, or both with quality settings
  * **Region Selector**: Customize overlay color and transparency with live preview
  * **After Recording**: Toggle save dialog and notifications

### 2. Start recording
Run the recorder:
./grec

  1. A dialog will appear asking you to select **Region Select** or **Full Screen**:
     * **Region Select**: Click and drag to select the area you want to record. The overlay color will match your preferences.
     * **Full Screen**: Recording starts immediately.
  2. A small **red blinking dot** (popup window, not in taskbar) indicates recording is active.
  3. Press **Ctrl + Shift + End** to stop recording.
  4. If enabled in preferences, a dialog will show where the file was saved with options:
     * **Open folder**: Opens your save directory.
     * **Close**: Closes the dialog.

Videos are saved according to your preferences:
  * Default directory: `~/Videos/`
  * Default pattern: `recording_YYYYMMDD_HHMMSS.mp4`

### Important Notes
  * **MP4 files are now Facebook-compatible!** They use H.264 codec with proper settings
  * The region selector automatically handles odd-sized selections - no more errors
  * The recording indicator won't appear in your taskbar or steal focus

* * *

## Configuration File

Both applications share the same configuration file at `~/.config/grec.conf`:

# Screen Recorder Configuration
save_format=mp4
save_directory=~/Videos
filename_pattern=recording_%Y%m%d_%H%M%S
framerate=30
video_quality=balanced
show_mouse=true
record_microphone=false
record_system=true
audio_quality=medium
region_r=0.98
region_g=0.38
region_b=0.008
region_alpha=0.2
show_save_dialog=true
show_notifications=true

* * *

## XFCE4 Integration

### Add to Panel
  1. Right-click on XFCE panel → **Panel** → **Add New Items**
  2. Choose **Launcher**
  3. Right-click the new launcher → **Properties**
  4. Set command to `/usr/local/bin/grec_prefs`
  5. Choose an icon (e.g., `gtk-preferences`)

OR

### Add to Application's Menu
  1. Ensure /usr/local/bin/grec_prefs exists
  2. Navigate to ~/.local/share/applications/
  3. nano ~/.local/share/applications/grec-preferences.desktop
  4. Paste the following:
```
[Desktop Entry]
Version=1.0
Type=Application
Name=Grec Settings
Comment=Configure Grec options
Exec=/usr/local/bin/grec_prefs
Icon=gtk-preferences
Terminal=false
Categories=AudioVideo;Settings;
StartupNotify=true
```

### Keyboard Shortcut
  1. Open **Settings Manager → Keyboard → Application Shortcuts**
  2. Click **Add**, enter `/usr/local/bin/grec` as the command
  3. Press your desired key combination (e.g., Ctrl+Shift+R)

Now you can start screen recording anywhere in XFCE with a single shortcut and configure it from the panel!

* * *

## Project Structure

Structure should looke like this after building:
```
grec/
├── grec           - Main recorder binary
├── grec_prefs     - Standalone preferences binary
├── main.cpp       - Recorder source code
├── grec_prefs.cpp - Preferences GUI source code
├── Makefile       - Build configuration
└── README.md      - This file
```

* * *

## Debugging

Output from FFmpeg is printed to the terminal for debugging:
Exiting normally, received signal 15

Ensure FFmpeg is installed and working:
ffmpeg -version
ffmpeg -encoders | grep libx264  # Should show x264 if properly configured

To see detected audio sources:
pactl list sources | grep -E "Name:|device.class"

* * *

## License

This project is licensed under the **MIT License**.

* * *

## About the Author

GREC was created by BigSlimThic, a hopelessly broke digital low-life who somehow grew up somewhere between Philadelphia and probably South East Asia, surviving on instant noodles and bad Wi-Fi. Rumor has it he has a smoking hot girlfriend, unless she left him for a guy with a real job. Against all odds, he somehow managed to survive the apocalypse of homelessness, poverty, and questionable life choices to create this.

### Donate
Help fund his lifelong quest to buy an ergonomic chair, a better Wi-Fi router, and possibly a vacation somewhere that isn't just his imagination.

BTC: 3GtCgHhMP7NTxsdNjcDs7TUNSBK6EXoAzz

ETH: 0x5f1ed610a96c648478a775644c9244bf4e78631e

* * *

## Changelog

### v2.1 (February 2026)
  * **Facebook compatibility**: Switched from mpeg4 to H.264 (libx264) codec for MP4 recordings
  * **Even dimensions fix**: Added scale filter to handle odd-sized region selections (required for H.264)
  * **Optimized encoding**: Added `-preset fast` and `-crf 23` for perfect quality/performance balance
  * **Taskbar fix**: Changed recording indicator to popup window with `GTK_WINDOW_POPUP`
  * **Focus prevention**: Added `gtk_window_set_accept_focus(FALSE)` to stop indicator from stealing focus

### v2.0 (February 2026)
  * Added standalone preferences application (`grec_prefs`)
  * Added format selection (MP4, MKV, GIF)
  * Added video quality presets
  * Added framerate control
  * Added system audio capture with microphone mixing
  * Added auto-detection of PulseAudio sources
  * Added customizable region selector color and transparency
  * Added live preview of overlay color
  * Added proper Makefile for easy compilation
  * Configuration now saved to `~/.config/grec.conf`

### v1.0 (February 2026)
  * Initial release
  * Full-screen and region recording
  * MKV output with MPEG4 codec
  * ALSA audio capture
  * Basic hotkey support

* * *

## About

GTK/X11 screen recorder with standalone preferences app, system audio capture, and customizable overlay.

**Resources**
  * Report Bug: https://github.com/mreinrt/grec/issues
  * Request Feature: https://github.com/mreinrt/grec/issues

License: MIT

Stars: 0
Watchers: 0
Forks: 0

## Releases
No releases published

## Packages
No packages published

## Languages
  * C++ 96.9%
  * Makefile 3.1%
