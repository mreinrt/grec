# grec

`grec` is a standalone GTK3/X11-based screen and audio recorder for Linux. It allows you to capture either your **full screen** or a **selected region** and records audio from your default ALSA device. Output is saved in **MKV format**.  

It is designed for desktop environments like XFCE, providing a quick and lightweight way to record your screen without complicated setup.

---

## Features

- Full-screen or region-based recording
- Audio capture from default ALSA input
- Output in MKV format (Matroska) for wide compatibility
- Transparent overlay for region selection
- Red blinking indicator for active recording
- Hotkey control (Ctrl + Shift + Print Screen) to start/stop
- Dialog boxes for capture mode selection and saved file notification
- X/close button stops recording automatically
- Lightweight and standalone; does not interfere with other desktop tools

---

## Requirements

- Linux (tested on Gentoo/XFCE4, should work on other distros with GTK3/X11)
- GTK3 development libraries
- X11 development libraries
- FFmpeg installed and available in PATH
- GCC (or compatible C++ compiler)

### Gentoo example:
sudo emerge media-video/ffmpeg dev-libs/gtk+:3.0 x11-libs/libX11

---

## Building

Clone or download the repository:

git clone <your-repo-url>
cd grec

Compile the program:

g++ recorder.cpp `pkg-config --cflags --libs gtk+-3.0` -lX11 -o grec

---

## Installation

(Optional) Move the binary to a directory in your `$PATH`:

sudo cp grec /usr/local/bin/
sudo chmod +x /usr/local/bin/grec

---

## Usage

Run the program:

./grec

1. Run grec in terminal (ensure grec is in /usr/local/bin or /usr/bin) 
2. A dialog will appear asking you to select **Region Select** or **Full Screen**:
   - **Region Select**: Click and drag to select the area you want to record.  
   - **Full Screen**: Recording starts immediately.  
3. A small **red blinking dot** indicates recording is active.  
4. Press **Ctrl + Shift + End** again to stop recording.  
5. After stopping, a dialog will show where the file was saved with options:
   - **Open folder**: Opens your `~/Videos` directory.  
   - **Close**: Closes the dialog.

All videos are saved to:

~/Videos/record_<timestamp>.mkv

---

## XFCE4 Integration

You can bind `grec` to a keyboard shortcut:

1. Open **Settings Manager → Keyboard → Application Shortcuts**  
2. Click **Add**, enter `/usr/local/bin/grec` as the command  
3. Press your desired key combination (e.g., Ctrl+Shift+Print Screen)  

Now you can start screen recording anywhere in XFCE with a single shortcut.

---

## Debugging

Output from FFmpeg is printed to the terminal for debugging:

Exiting normally, received signal 15

Ensure FFmpeg is installed and working:

ffmpeg -version

---

## License

This project is licensed under the **MIT License**.

---

GREC was created by BigSlimThic, a hopelessly broke digital low-life who somehow grew up somewhere between Philadelphia and probably South East Asia, surviving on instant noodles and bad Wi-Fi. Rumor has it he has a smoking hot girlfriend, unless she left him for a guy with a real job. Against all odds, he somehow managed to survive the apocalypse of homelessness, poverty, and questionable life choices to create this AI.

Donate to BigSlimThic: Help fund his lifelong quest to buy an ergonomic chair, a better Wi-Fi router, and possibly a vacation somewhere that isn't just his imagination.

BTC: 3GtCgHhMP7NTxsdNjcDs7TUNSBK6EXoAzz

ETH: 0x5f1ed610a96c648478a775644c9244bf4e78631e
