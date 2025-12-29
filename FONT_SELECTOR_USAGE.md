# Font Selector Usage

## How to Use the Font Selector

The visual font selector has been implemented! Here's how to use it:

### Opening the Font Selector

1. **Via Menu**: Click the "Settings" button in the menu bar
2. **Keyboard**: The settings menu can be accessed via keyboard shortcuts (once menu bar keyboard shortcuts are fully implemented)

### Font Selector Features

#### Font List
- **Scrollable list** of all TTF/OTF fonts found in the `fonts/` directory
- **Click a font** to select it (highlighted)
- **Arrow keys** (Up/Down) to navigate the list
- Selected font is highlighted

#### Font Size Control
- **Decrease button (-)**: Reduce font size (minimum 8pt)
- **Increase button (+)**: Increase font size (maximum 72pt)
- Current size displayed between buttons

#### Buttons
- **Cancel**: Close dialog without saving changes
- **Apply**: Save selected font and size, then close dialog
- **ESC key**: Same as Cancel
- **Enter key**: Same as Apply

### Font Discovery

The font selector automatically discovers fonts from:
1. `fonts/` directory (relative to where you run the program)
2. `~/hyperterm/fonts/` (absolute path fallback)

It finds all `.ttf` and `.otf` files in these directories.

### Current Limitations

⚠️ **Visual Rendering**: The UI structure and logic are complete, but the actual visual rendering (drawing rectangles, text) is commented out because vertex buffer rendering isn't fully implemented yet. Once vertex buffers are added, uncomment the rendering calls in:
- `renderDialog()`
- `renderFontList()`
- `renderFontSizeControl()`
- `renderButtons()`

The functionality (clicking, selection, saving) works, but you won't see the visual UI until rendering is complete.

### Settings Saved

When you click "Apply", the settings are saved to:
```
~/.hyperterm/config
```

Format:
```
font.path=fonts/YourFontName.ttf
font.size=16
```

### Example Usage Flow

1. Place fonts in `fonts/` directory
2. Run Hyperterm
3. Click "Settings" in menu bar
4. Font selector dialog appears (once rendering is implemented)
5. Click on a font name to select it
6. Use +/- buttons to adjust size
7. Click "Apply" to save
8. Settings are saved and font is loaded

