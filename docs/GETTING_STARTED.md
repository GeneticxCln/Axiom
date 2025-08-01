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

### Core Window Management
- `Super + Q`: Quit compositor
- `Super + T`: Toggle tiling mode
- `Super + Return`: Launch terminal
- `Super + D`: Launch rofi application launcher
- `Super + B`: Toggle waybar status bar
- `Super + W` / `Alt + F4`: Close focused window
- `Super + F`: Toggle fullscreen
- `Super + L`: Cycle tiling layouts (Grid → Master-Stack → Spiral → Floating)
- `Super + Space`: Toggle focused window floating/tiled
- `Super + H/J`: Adjust master window ratio
- `Alt + Tab`: Cycle through windows
- `Escape`: Cancel interactive operations

### Phase 2.1: Advanced Workspace Management
- **`Super + 1-9`**: Switch directly to workspace 1-9
- **`Super + Shift + 1-9`**: Move focused window to workspace 1-9
- **Named Workspaces**: Main, Web, Code, Term, Media, Files, Chat, Game, Misc, Temp
- **Layout Persistence**: Each workspace remembers its layout and master ratio

## Window Decorations
- **Title Bars**: Dark gray title bars with window titles
- **Borders**: Configurable colored borders around windows
- **Focus Indicators**: Visual feedback for focused windows
- **Rounded Corners**: Smooth, modern window appearance

## Configuration
Currently uses hardcoded settings. Configuration file support planned.
