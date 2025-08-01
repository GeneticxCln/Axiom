<div align="center">
  <img src="logo.svg" alt="Axiom Logo" width="200" height="200"/>
  
  # Axiom
  
  **A modern, feature-rich Wayland compositor built with wlroots**
  
  ![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)
  ![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
  ![Wayland](https://img.shields.io/badge/Wayland-Compatible-green.svg)
  
</div>

Axiom is a lightweight yet powerful Wayland compositor designed for productivity and customization. Built on top of wlroots, it provides advanced window management features including intelligent window snapping, multi-monitor support, and seamless integration with existing Wayland applications.

## ✨ Features

### 🪟 **Advanced Window Management**
- **Intelligent Window Snapping**: Magnetic window edges with configurable snap zones
- **Smart Corner Detection**: Automatic corner snapping for precise window positioning
- **Multi-Monitor Support**: Seamless window management across multiple displays
- **Window-to-Window Snapping**: Snap windows relative to other windows
- **Edge Resistance**: Configurable resistance when moving windows near screen edges

### 🎯 **Performance & Compatibility**
- **Hardware Acceleration**: Optimized rendering with GPU acceleration
- **Wayland Native**: Full Wayland protocol support with XWayland compatibility
- **Memory Efficient**: Lightweight design with minimal resource usage
- **wlroots Foundation**: Built on the proven wlroots library for stability

### ⚙️ **Customization**
- **Configurable Snapping**: Adjustable snap thresholds and magnetism strength
- **Flexible Layout**: Support for various window management paradigms
- **Extensible Architecture**: Clean API for adding custom features

## 🚀 Quick Start

### Prerequisites

Ensure you have the following dependencies installed:

**Arch Linux / CachyOS:**
```bash
sudo pacman -S wayland wayland-protocols wlroots0.19 pixman libdrm meson ninja gcc
```

**Debian/Ubuntu:**
```bash
sudo apt-get install wayland-protocols libwayland-dev libwlroots-dev libpixman-1-dev libdrm-dev meson ninja-build gcc
```

### Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/GeneticxCln/Axiom.git
   cd Axiom
   ```

2. **Generate protocol headers:**
   ```bash
   wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
   ```

3. **Build with Meson:**
   ```bash
   meson setup build
   meson compile -C build
   ```

4. **Install (optional):**
   ```bash
   sudo meson install -C build
   ```

### Running Axiom

**Run in nested mode (for testing):**
```bash
./build/axiom
```

**Run as main compositor:**
```bash
# From TTY (Ctrl+Alt+F2)
./build/axiom
```

## 🔧 Configuration

Axiom's window snapping system is highly configurable. The default configuration provides sensible defaults, but you can customize behavior through the configuration API.

### Window Snapping Configuration

```c
// Example configuration
axiom_window_snapping_config config = {
    .snap_threshold = 20,           // Pixels from edge to trigger snap
    .edge_resistance = 10,          // Resistance when moving near edges
    .magnetism_strength = 0.75f,    // Magnetic pull strength (0.0-1.0)
    .smart_corners_enabled = true,  // Enable intelligent corner detection
    .multi_monitor_enabled = true,  // Enable cross-monitor snapping
    .window_to_window_enabled = true, // Enable window-to-window snapping
    .edge_snapping_enabled = true,  // Enable edge snapping
    .enabled = true                 // Master enable/disable
};
```

### Snap Directions

Axiom supports comprehensive snapping in all directions:
- **Edge Snapping**: TOP, BOTTOM, LEFT, RIGHT
- **Corner Snapping**: TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT
- **Center Snapping**: CENTER (for monitor center alignment)
- **Free Movement**: NONE (no snapping)

## 🏗️ Architecture

### Core Components

```
axiom/
├── src/
│   ├── main.c              # Main compositor entry point
│   ├── window_snapping.c   # Window snapping implementation
│   └── ...
├── include/
│   ├── axiom.h            # Main header with core definitions
│   ├── window_snapping.h  # Window snapping API
│   └── ...
└── tests/
    ├── test_snapping.c           # Comprehensive snapping tests
    └── test_snapping_simple.c    # Standalone snapping tests
```

### Window Snapping System

The window snapping system is built around a manager-based architecture:

1. **Manager Initialization**: `axiom_window_snapping_manager_create()`
2. **Configuration**: `axiom_window_snapping_update_config()`
3. **Runtime Operations**: Automatic snap detection and application
4. **Statistics**: `axiom_window_snapping_get_stats()` for monitoring
5. **Cleanup**: `axiom_window_snapping_manager_destroy()`

## 🧪 Testing

Axiom includes comprehensive tests to verify functionality:

### Running Tests

```bash
# Compile and run the standalone snapping test
gcc -DWLR_USE_UNSTABLE -I./include -I. -I/usr/include/wlroots-0.19 \
    -I/usr/include/pixman-1 -I/usr/include/libdrm \
    test_snapping_simple.c src/window_snapping.c \
    -o test_snapping_simple -lwayland-server -lwlroots-0.19

./test_snapping_simple
```

### Test Coverage

- ✅ Manager creation and destruction
- ✅ Configuration system validation
- ✅ Statistics tracking accuracy
- ✅ Memory management verification
- ✅ All snap directions functionality
- ✅ Edge case handling

## 🤝 Contributing

We welcome contributions to Axiom! Please follow these guidelines:

### Development Setup

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes with appropriate tests
4. Ensure all tests pass
5. Commit with descriptive messages
6. Push to your fork: `git push origin feature/amazing-feature`
7. Open a Pull Request

### Code Style

- Follow existing code style and conventions
- Include comprehensive comments for complex logic
- Add tests for new features
- Update documentation as needed

### Reporting Issues

Please use GitHub Issues to report bugs or request features. Include:
- Detailed description of the issue
- Steps to reproduce
- System information (distribution, wlroots version, etc.)
- Relevant log output

## 📊 Performance

Axiom is designed for optimal performance:

- **Low Memory Footprint**: Minimal RAM usage even with multiple windows
- **Efficient Snap Detection**: O(1) snap calculations for smooth window movement
- **Hardware Acceleration**: GPU-accelerated rendering where available
- **Responsive Input**: Sub-millisecond input handling

## 🔍 Troubleshooting

### Common Issues

**Cursor conflicts in nested mode:**
- This is a known limitation when running nested inside other Wayland compositors
- Use software cursor rendering: `WLR_NO_HARDWARE_CURSORS=1 ./axiom`

**Missing protocol headers:**
- Generate required headers: `wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h`

**Build errors:**
- Ensure all dependencies are installed
- Check wlroots version compatibility (supports 0.19+)

## 📜 License

Axiom is released under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- **wlroots**: The foundational library that makes Axiom possible
- **Wayland**: The modern display server protocol
- **Hyprland**: Inspiration for advanced window management features
- **sway**: Reference implementation for Wayland compositor architecture

## 📞 Support

- **Documentation**: [GitHub Wiki](https://github.com/GeneticxCln/Axiom/wiki)
- **Issues**: [GitHub Issues](https://github.com/GeneticxCln/Axiom/issues)
- **Discussions**: [GitHub Discussions](https://github.com/GeneticxCln/Axiom/discussions)

---

**Made with ❤️ for the Linux desktop community**

