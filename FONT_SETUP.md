# Font Setup Guide

## Where to Put Fonts

Place your font files (TTF or OTF format) in the **`fonts/`** directory:

```
hyperterm/
└── fonts/
    ├── default.ttf          # Default font (or any name you want)
    ├── EBGaramond-Regular.ttf
    ├── CrimsonText-Regular.ttf
    └── ... (other fonts)
```

## Font Selector Status

⚠️ **The visual font selector UI is not yet implemented** - it's a placeholder in the code.

However, you can configure fonts in two ways:

### Method 1: Config File (Current Way)

The settings are stored in: `~/.hyperterm/config`

Create or edit this file:

```bash
mkdir -p ~/.hyperterm
nano ~/.hyperterm/config
```

Add these lines:

```
font.path=fonts/EBGaramond-Regular.ttf
font.size=16
```

**Note:** The path is relative to where you run the program from, or use an absolute path:
```
font.path=/home/davidjackson/hyperterm/fonts/EBGaramond-Regular.ttf
```

### Method 2: Settings UI (Not Yet Implemented)

The Settings menu exists (press the Settings button or menu item), but the visual UI for selecting fonts hasn't been implemented yet. This is on the TODO list.

## Recommended Antique Serif Fonts

1. **EB Garamond** - Classic serif
   - Download: https://fonts.google.com/specimen/EB+Garamond
   - Place `EBGaramond-Regular.ttf` in `fonts/` directory

2. **Crimson Text** - Elegant serif
   - Download: https://fonts.google.com/specimen/Crimson+Text

3. **Lora** - Readable serif
   - Download: https://fonts.google.com/specimen/Lora

4. **Playfair Display** - Decorative serif
   - Download: https://fonts.google.com/specimen/Playfair+Display

## Quick Setup Example

```bash
# Download a font (example: EB Garamond)
cd /home/davidjackson/hyperterm/fonts
wget https://github.com/georgd/EB-Garamond/releases/download/v0.016/EBGaramond-0.016.zip
unzip EBGaramond-0.016.zip
# Copy the font file to fonts directory
cp EBGaramond-0.016/ttf/EBGaramond-Regular.ttf .

# Create config file
mkdir -p ~/.hyperterm
echo "font.path=fonts/EBGaramond-Regular.ttf" > ~/.hyperterm/config
echo "font.size=16" >> ~/.hyperterm/config
```

## System Fonts

You can also use system fonts by using absolute paths:

```
font.path=/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf
```

Or find system fonts:
```bash
fc-list : family | grep -i serif
```

## Current Limitations

- ⚠️ Font selector UI not implemented (use config file)
- ⚠️ Font loading uses FreeType but currently creates placeholder glyphs
- ⚠️ Font rendering needs full FreeType integration (see REMAINING_WORK.md)

The font path is read from settings, but actual font rendering with FreeType needs to be completed.

