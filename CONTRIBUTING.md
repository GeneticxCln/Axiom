# Contributing to Axiom

Thank you for your interest in contributing to Axiom! We welcome contributions from developers of all skill levels.

## 🚀 Quick Start

### Development Setup

1. **Fork the repository**
   ```bash
   git clone https://github.com/YourUsername/Axiom.git
   cd Axiom
   ```

2. **Install dependencies** (Arch Linux/CachyOS)
   ```bash
   sudo pacman -S wayland wayland-protocols wlroots0.19 pixman libdrm meson ninja gcc
   ```

3. **Build the project**
   ```bash
   wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h
   meson setup build
   meson compile -C build
   ```

4. **Run tests**
   ```bash
   # Manual test execution (meson test has path issues)
   ./build/test_axiom
   ./build/test_effects
   ./build/test_animation
   ./build/test_config
   ./build/test_window_rules
   ```

5. **Test the compositor**
   ```bash
   # Run in nested mode for development
   ./build/axiom --nested
   ```

## 📋 How to Contribute

### Reporting Issues

- Use [GitHub Issues](https://github.com/GeneticxCln/Axiom/issues)
- Include system information (distribution, wlroots version, GPU)
- Provide detailed reproduction steps
- Attach relevant log output

### Pull Requests

1. **Create a feature branch**
   ```bash
   git checkout -b feature/amazing-feature
   ```

2. **Make your changes**
   - Follow the existing code style
   - Add tests for new features
   - Update documentation as needed

3. **Test thoroughly**
   - Ensure all existing tests pass
   - Test your changes in nested mode
   - Verify no memory leaks or crashes

4. **Commit with clear messages**
   ```bash
   git commit -m "feat: add window transparency controls
   
   - Implement per-window opacity settings  
   - Add configuration options for focus states
   - Include tests for transparency system"
   ```

5. **Push to your fork**
   ```bash
   git push origin feature/amazing-feature
   ```

6. **Open a Pull Request**
   - Use the provided PR template
   - Reference any related issues
   - Include screenshots/videos for visual changes

## 🎯 Development Areas

### 🔥 High Priority
- **Performance optimizations** for GPU effects
- **Memory leak fixes** in window management
- **Multi-monitor enhancements** for window snapping
- **Configuration validation** improvements
- **Error handling** robustness

### 🚀 New Features
- **Window thumbnails** in taskbar/dock integration
- **Advanced animations** (elastic, bounce effects)
- **Plugin system** for extensibility
- **IPC interface** for external tools
- **Touch gesture support** for laptops

### 🐛 Bug Fixes
- Check [GitHub Issues](https://github.com/GeneticxCln/Axiom/issues) for open bugs
- Focus on memory safety and crash prevention
- Improve edge case handling in window management

## 🎨 Code Style Guidelines

### C Code Standards
- **C11 standard**
- **4-space indentation** (no tabs)
- **Snake_case** for functions and variables
- **PascalCase** for structs and types
- **Clear, descriptive names**

### Code Organization
```c
// Headers in order: system, wlroots, local
#include <stdio.h>
#include <wlr/types/wlr_compositor.h>
#include "axiom.h"

// Function documentation
/**
 * Initialize the window snapping system
 * 
 * @param server The main server instance
 * @return true on success, false on failure
 */
bool axiom_window_snapping_init(struct axiom_server *server);
```

### Memory Management
- **Always check malloc/calloc return values**
- **Free all allocated memory**
- **Use proper cleanup functions**
- **Avoid memory leaks in error paths**

### Error Handling
```c
// Good error handling pattern
if (!some_function()) {
    AXIOM_LOG_ERROR("Failed to initialize component");
    cleanup_resources();
    return false;
}
```

## 🧪 Testing Guidelines

### Unit Tests
- **Test all new functions**
- **Include edge cases**
- **Test error conditions**
- **Maintain high test coverage**

### Integration Testing
```bash
# Test in various environments
./build/axiom --nested          # Nested mode
WLR_NO_HARDWARE_CURSORS=1 ./build/axiom  # Software cursors
./build/axiom --debug           # Debug mode
```

### Performance Testing
- **Profile with valgrind** for memory leaks
- **Monitor GPU usage** during effects
- **Test with multiple windows/workspaces**
- **Verify 60fps performance**

## 📚 Documentation

### Code Documentation
- **Document all public functions**
- **Explain complex algorithms**
- **Include usage examples**
- **Keep comments up to date**

### User Documentation
- Update README.md for new features
- Add configuration examples
- Include troubleshooting steps
- Write clear installation instructions

## 🌟 Recognition

Contributors are recognized in:
- **CHANGELOG.md** for significant contributions
- **GitHub contributors page**
- **Release notes** for major features
- **Documentation credits**

## 📞 Getting Help

- **GitHub Discussions** for questions
- **GitHub Issues** for bugs and feature requests
- **Code review feedback** in pull requests
- **Development chat** (link TBD)

## 📜 Code of Conduct

- **Be respectful** to all contributors
- **Provide constructive feedback**
- **Help newcomers** get started
- **Focus on technical merit**
- **Maintain a welcoming environment**

## 🎯 Contribution Areas by Skill Level

### 🟢 Beginner Friendly
- Documentation improvements
- Configuration validation
- Error message improvements
- Simple bug fixes
- Test case additions

### 🟡 Intermediate
- Window management features
- Configuration system enhancements  
- Input handling improvements
- Animation system extensions
- Memory optimization

### 🔴 Advanced
- GPU rendering optimizations
- wlroots integration improvements
- Complex window effects
- Performance profiling
- Architecture changes

---

## 🚀 Ready to Contribute?

1. Check [open issues](https://github.com/GeneticxCln/Axiom/issues)
2. Look for "good first issue" labels
3. Comment on issues you'd like to work on
4. Fork the repository and start coding!

**Thank you for helping make Axiom the best Wayland compositor!** 🎉
