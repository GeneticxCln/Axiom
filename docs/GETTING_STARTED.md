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
- `Super + F1`: Focus next window
- `Super + Return`: Launch terminal
- `Super + D`: Launch rofi application launcher
- `Super + B`: Toggle waybar status bar
- `Super + W`: Close focused window
- `Escape`: Cancel interactive operations

## Configuration
Currently uses hardcoded settings. Configuration file support planned.
