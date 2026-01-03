# Evades Overlay

A stylish Windows overlay for displaying keyboard inputs and mouse position. Perfect for streaming, recording gameplay, or creating content for games like Evades.io.

![Windows](https://img.shields.io/badge/Platform-Windows-blue)
![C++](https://img.shields.io/badge/Language-C%2B%2B17-orange)

## ✨ Features

- **Keyboard Input Display** - Shows keys 1-5, WASD, ZXCS, Shift with smooth animations
- **Mouse Trail** - Winter-themed trail with snowflake particles following your cursor
- **RGB Effects** - Rainbow cycling border and key highlights
- **Transparent Overlay** - Click-through window that stays on top
- **OBS Compatible** - Works with streaming and recording software

## 📋 Requirements

### Build Requirements

- **Windows 10/11**
- **Microsoft Visual Studio 2022** (Community, Professional, or Enterprise)
  - During installation, select the **"Desktop development with C++"** workload

### Runtime Requirements

- **Windows 10/11** (uses GDI+ for rendering)

## 🔧 Building

### Option 1: Using compile.bat (Recommended)

1. Open a terminal/command prompt in the project folder
2. Run:
   ```batch
   compile.bat
   ```
3. The executable `EvadesOverlay.exe` will be created

### Option 2: Manual Compilation

```batch
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cl /EHsc /D_UNICODE /DUNICODE /std:c++17 EvadesOverlay.cpp /Fe:EvadesOverlay.exe gdiplus.lib user32.lib gdi32.lib
```

### ⚠️ Troubleshooting

If Visual Studio is installed in a **non-default location** or you have a different edition:

1. Open `compile.bat` in a text editor
2. Edit line 4 to match your `vcvarsall.bat` path:
   - **Professional**: `C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat`
   - **Enterprise**: `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat`
   - **Custom Path**: Adjust accordingly

## 🚀 Usage

1. Run `EvadesOverlay.exe`
2. The overlay appears in the top-left corner of your screen
3. **Drag** the title bar to move the overlay
4. Press **ESC** to close the overlay

### Controls Displayed

| Overlay Key | Keyboard Input |
|-------------|----------------|
| 1-5 | Number keys 1-5 |
| W | W or ↑ Arrow |
| A | A or ← Arrow |
| S | S or ↓ Arrow |
| D | D or → Arrow |
| Z | Z or J |
| X | X or K |
| C | C |
| SHIFT | Left or Right Shift |

## 📝 License

Feel free to use and modify for personal projects.

## 🎮 Made for

Originally designed for [Evades.io](https://evades.io) gameplay recording.
