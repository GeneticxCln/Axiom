# Axiom Wayland Compositor - User Guide

## 🚀 Welcome to Axiom

Axiom is a professional-grade Wayland compositor that combines the productivity of tiling window managers like dwm with modern graphics capabilities and GPU acceleration.

## 📋 Table of Contents

1. [Quick Start](#quick-start)
2. [Configuration](#configuration)
3. [Keybindings](#keybindings)
4. [Window Management](#window-management)
5. [Multi-Monitor Setup](#multi-monitor-setup)
6. [Effects and Theming](#effects-and-theming)
7. [Troubleshooting](#troubleshooting)

## 🚀 Quick Start

### Installation

1. **Build from source:**
   ```bash
   git clone https://github.com/GeneticxCln/Axiom.git
   cd Axiom
   meson setup builddir
   meson compile -C builddir
   sudo meson install -C builddir
   ```

2. **Test in nested mode:**
   ```bash
   ./builddir/axiom --nested
   ```

3. **Run as main compositor:**
   ```bash
   # From TTY (Ctrl+Alt+F2)
   axiom
   ```

### First Steps

After launching Axiom:
- Press `Super+Return` to open a terminal
- Press `Super+d` to launch application launcher
- Press `Super+1-9` to switch between tags
- Press `Super+q` to quit

## ⚙️ Configuration

### Configuration Files

Axiom uses INI-based configuration files located in `~/.config/axiom/`:

- `axiom.conf` - Main configuration
- `rules.conf` - Window-specific rules

### Main Configuration (`~/.config/axiom/axiom.conf`)

```ini
[input]
repeat_rate = 25
repeat_delay = 600
cursor_theme = "default"
cursor_size = 24

[tiling]
enabled = true
border_width = 2
gap_size = 5
master_ratio = 0.6

[appearance]
background_color = "#1e1e1e"
border_active = "#ffffff" 
border_inactive = "#666666"

[animations]
enabled = true
window_animations = true
workspace_animations = true
speed_multiplier = 1.0
default_easing = "ease_out_cubic"

[effects]
shadows_enabled = true
blur_enabled = true
transparency_enabled = true
shadow_blur_radius = 10
blur_radius = 15
focused_opacity = 1.0
unfocused_opacity = 0.85

[smart_gaps]
enabled = true
base_gap = 10
single_window_gap = 0
adaptive_mode = "count"

[window_snapping]
enabled = true
snap_threshold = 20
edge_resistance = 5
smart_corners = true
```

### Window Rules (`~/.config/axiom/rules.conf`)

```ini
[firefox]
workspace = 1
position = maximized

[code]
workspace = 2
size = 1200x800
position = center

[mpv]
floating = true
size = 640x480
position = bottom-right

[calculator]
floating = true
size = 300x400
position = top-right
```

## ⌨️ Keybindings

### Window Management
- `Super+Return` - Launch terminal
- `Super+d` - Launch application launcher
- `Super+w` - Close window
- `Super+f` - Toggle fullscreen
- `Super+space` - Toggle floating/tiled
- `Super+s` - Toggle sticky (appears on all tags)

### Tag Management (dwm-style)
- `Super+1-9` - View specific tag
- `Super+Shift+1-9` - Move window to tag
- `Super+Ctrl+1-9` - Toggle tag visibility
- `Super+Ctrl+Shift+1-9` - Toggle window tag assignment
- `Super+0` - View all tags
- `Super+Tab` - View previous tags

### Layout Management
- `Super+l` - Cycle through layouts
- `Super+h/j` - Adjust master area ratio
- `Super+Shift+h/j` - Adjust master window count
- `Super+Ctrl+1-4` - Set specific layout:
  - Ctrl+1: Master-stack
  - Ctrl+2: Grid
  - Ctrl+3: Spiral
  - Ctrl+4: Binary tree

### Focus Management
- `Alt+Tab` - Cycle through windows forward
- `Alt+Shift+Tab` - Cycle through windows backward

### System Controls
- `Super+q` - Quit compositor
- `Super+r` - Reload configuration

### Gap Management
- `Super+equal` - Increase window gaps
- `Super+minus` - Decrease window gaps

## 🪟 Window Management

### Tagging System

Axiom uses a powerful tagging system inspired by dwm:

**What are tags?**
- Tags are like workspaces but more flexible
- Windows can belong to multiple tags simultaneously
- You can view multiple tags at once

**Tag Operations:**
```bash
# View tag 1
Super+1

# Move current window to tag 2
Super+Shift+2

# View both tags 1 and 3
Super+1, then Super+Ctrl+3

# Make window appear on tags 1 and 2
Super+Shift+1, then Super+Ctrl+Shift+2
```

### Layout Modes

1. **Master-Stack** (default)
   - One large master area on the left
   - Stack of windows on the right
   - Adjustable master ratio

2. **Grid**
   - Windows arranged in a grid pattern
   - Automatically adjusts based on window count

3. **Spiral**
   - Windows arranged in a spiral pattern
   - Visually appealing for many windows

4. **Binary Tree**
   - Hierarchical window arrangement
   - Efficient space utilization

### Floating Windows

Some windows work better as floating:
- Calculators, dialogs, popup windows
- Use `Super+space` to toggle floating mode
- Floating windows can be moved and resized normally

### Sticky Windows

Sticky windows appear on all tags:
- Perfect for monitoring tools, chat applications
- Use `Super+s` to toggle sticky mode
- Appears regardless of current tag selection

## 🖥️ Multi-Monitor Setup

### Automatic Detection

Axiom automatically detects connected monitors and configures them appropriately.

### Per-Monitor Configuration

Each monitor has independent:
- Tiling engine and layout
- Workspace/tag management
- Window arrangements
- Configuration settings

### Monitor Commands

```bash
# Move window to specific monitor
# (Use mouse or window rules)

# Configure monitor in rules.conf
[work-monitor]
output = "DP-1"
workspace = 1
position = left

[media-monitor]
output = "HDMI-1" 
workspace = 9
position = right
```

### Workspace Distribution

- Each monitor can have its own active tags
- Windows can be moved between monitors
- Consistent keybindings work across all monitors

## 🎨 Effects and Theming

### Visual Effects

**Shadows:**
- Configurable blur radius and offset
- Custom shadow colors and opacity
- GPU-accelerated rendering

**Blur:**
- Gaussian blur for unfocused windows
- Configurable blur radius and intensity
- Two-pass blur for optimal quality

**Transparency:**
- Focus-based opacity changes
- Separate settings for focused/unfocused/inactive
- Smooth transitions

**Animations:**
- Window appear/disappear animations
- Workspace switching animations
- Multiple easing functions available

### Performance Settings

```ini
[effects]
# Disable effects for better performance
shadows_enabled = false
blur_enabled = false
transparency_enabled = false

# Or reduce quality
shadow_blur_radius = 5    # Lower = faster
blur_radius = 8          # Lower = faster
```

## 🔧 Troubleshooting

### Common Issues

**Compositor won't start:**
```bash
# Check dependencies
axiom --version

# Run with debug output
axiom --debug --nested

# Check Wayland socket
echo $WAYLAND_DISPLAY
```

**Performance issues:**
```bash
# Disable effects temporarily
# Edit ~/.config/axiom/axiom.conf
[effects]
shadows_enabled = false
blur_enabled = false

# Check GPU acceleration
glxinfo | grep "direct rendering"
```

**Keybindings not working:**
```bash
# Check configuration syntax
axiom --check-config

# Reload configuration
Super+r
```

**Applications not launching:**
```bash
# Ensure environment variables are set
echo $XDG_RUNTIME_DIR
echo $WAYLAND_DISPLAY

# For X11 apps, ensure XWayland is working
xwininfo  # Should work in Axiom
```

### Log Files

Check logs for debugging:
```bash
# System logs
journalctl -u axiom

# Session logs (if using display manager)
~/.xsession-errors
```

### Getting Help

1. **Documentation:** Check docs/ directory
2. **Issues:** GitHub Issues for bug reports
3. **Discussions:** GitHub Discussions for questions
4. **Community:** Join Wayland compositor communities

## 📈 Performance Tips

### Optimization Settings

```ini
[animations]
# Reduce animation complexity
speed_multiplier = 2.0    # Faster animations
window_animations = false # Disable if needed

[effects]
# Optimize for older hardware
shadow_blur_radius = 5
blur_radius = 8
```

### Hardware Requirements

**Minimum:**
- OpenGL ES 2.0 support
- 2GB RAM
- Any modern GPU (Intel HD, AMD, NVIDIA)

**Recommended:**
- OpenGL ES 3.0 support  
- 4GB+ RAM
- Dedicated GPU for best effects performance

## 🎯 Pro Tips

### Productivity Workflows

1. **Developer Setup:**
   - Tag 1: Browser (research)
   - Tag 2: Code editor
   - Tag 3: Terminal
   - Tag 4: Documentation

2. **Media Workflow:**
   - Tag 8: Media player (sticky)
   - Tag 9: File manager
   - Multiple tags for different projects

3. **Monitoring Setup:**
   - Sticky terminal for system monitoring
   - Floating calculator for quick calculations
   - Multiple monitors for different contexts

### Advanced Configuration

```ini
# Fine-tune master ratio for your workflow
[tiling]
master_ratio = 0.7  # Larger main window

# Customize gaps for your preference
[smart_gaps]
base_gap = 15       # More breathing room
single_window_gap = 5  # Small gap for single windows

# Optimize animations for your hardware
[animations]
window_appear_duration = 200  # Faster appearance
workspace_switch_duration = 300  # Smooth switching
```

---

## 🎉 Conclusion

Axiom combines the efficiency of tiling window managers with modern compositor features. Take time to customize the configuration to match your workflow, and enjoy the productivity boost of keyboard-driven window management with beautiful visual effects!

For more advanced topics, check the other documentation files in the `docs/` directory.

---

*Happy tiling! 🚀*
