# Getting Started with Axiom

## Prerequisites
- wlroots-0.19 or later
- wayland-server
- xkbcommon
- meson and ninja

## Building
```bash
meson setup build
meson compile -C build
```

## Running
```bash
# In a TTY (Ctrl+Alt+F2)
./build/axiom

# Nested in existing Wayland session
./build/axiom --nested
```

## Keyboard Shortcuts
- `Super + Q`: Quit compositor
- `Super + T`: Toggle tiling mode
- `Super + F1`: Focus next window (legacy)
- `Alt + Tab`: Cycle through windows
- `Super + Return`: Launch terminal
- `Super + D`: Launch rofi application launcher
- `Super + B`: Toggle waybar status bar
- `Super + W` / `Alt + F4`: Close focused window
- `Super + F`: Toggle fullscreen
- `Escape`: Cancel interactive operations

## Window Decorations
- **Title Bars**: Dark gray title bars with window titles
- **Borders**: Configurable colored borders around windows
- **Focus Indicators**: Visual feedback for focused windows
- **Rounded Corners**: Smooth, modern window appearance

## Configuration
Currently uses hardcoded settings. Configuration file support planned.
