# Axiom Wayland Compositor v4.3.4 - Session Stability Release

**Release Date:** August 3, 2025  
**Tag:** `v4.3.4`  
**Focus:** SDDM Compatibility & Session Management Fixes

## 🚨 Critical Issues Fixed

This release addresses critical session management issues that were preventing Axiom from starting properly via SDDM (Simple Desktop Display Manager) and other display managers.

### The Problem
Users experienced:
- **Black screen with white line** when selecting Axiom from SDDM
- **Session crashes** that kicked users back to the login screen
- **Infinite cleanup loops** in session logs
- **Permission errors** when accessing GPU resources

### The Solution
We've completely overhauled the session management system with comprehensive fixes.

## 🐛 Critical Session Fixes

### Infinite Cleanup Loop Fix
- **Issue**: Session script was stuck in infinite cleanup loop causing immediate crashes
- **Fix**: Added `AXIOM_CLEANUP_DONE` flag to prevent multiple cleanup calls
- **Impact**: Sessions now start and terminate cleanly without loops

### SDDM Compatibility
- **Issue**: Session script incompatible with SDDM's process management
- **Fix**: Improved error handling, removed `set -e`, better session detection
- **Impact**: Axiom now works properly when launched from SDDM login screen

### GPU Access Permissions
- **Issue**: Users lacked proper permissions for GPU/DRM device access
- **Fix**: Automatically add users to `render` group during installation
- **Impact**: Hardware acceleration and compositor functionality now work properly
- **Note**: Requires logout/login for group membership to take effect

### Session Script Error Handling
- **Issue**: Script would crash on any non-critical error
- **Fix**: Removed strict error handling, added graceful error recovery
- **Impact**: Session continues running even if non-essential components fail

## 🛠️ Session Management Improvements

### Enhanced Debug Capabilities
- **New**: `axiom-session-debug` script for comprehensive troubleshooting
- **Features**: 
  - Environment validation
  - Permission checking
  - DRM device detection
  - Strace integration for advanced debugging
  - Detailed error reporting

### Better Session Detection
- **Improvement**: Smart detection between nested and primary display server modes
- **Benefit**: Automatically adapts to running environment (standalone vs nested)
- **Compatibility**: Works in both development and production scenarios

### Libseat & DRM Handling
- **Enhancement**: Better error messages for seat management failures
- **Fix**: Proper handling of "Device or resource busy" errors
- **Result**: Clear indication when compositor can't access display hardware

### Session Startup Recovery
- **Addition**: Timeout mechanisms to prevent hanging sessions
- **Feature**: Automatic fallback and recovery procedures
- **Logging**: Structured error reporting for easier troubleshooting

## 🔧 Development & Installation Improvements

### Build System Integration
- **Update**: `meson.build` now properly installs `axiom-session` script
- **Benefit**: Session script automatically deployed during `ninja install`
- **Path**: Installs to `/usr/local/bin/axiom-session` with correct permissions

### Enhanced Installation Scripts
- **Update**: `install-axiom-v3.5.1.sh` includes session script deployment
- **Addition**: Automatic user group management
- **Verification**: Installation process validates all components

### Advanced Debugging Tools
- **New**: Strace integration for system call tracing
- **Dependency**: Added `strace` package for debugging capabilities
- **Usage**: `./axiom-session-debug` provides comprehensive diagnostic output

### Structured Logging
- **Improvement**: Better log organization and error categorization
- **Location**: All logs centralized in `~/.cache/axiom/logs/`
- **Format**: Timestamped entries with severity levels

## 📋 Technical Details

### Modified Files
- `axiom-session` - Core session management script
- `meson.build` - Build system configuration
- `install-axiom-v3.5.1.sh` - Installation script
- `CHANGELOG.md` - Project changelog

### New Files Added
- `axiom-session-debug` - Comprehensive debugging script
- `test-primary-mode` - Primary display server testing tool
- `RELEASE_NOTES_v4.3.4.md` - This release documentation

### Dependencies Added
- `strace` - System call tracing for debugging
- Render group membership - Required for GPU access

### Compatibility Notes
- **Primary Display Mode**: Fixed seat management for standalone operation
- **Nested Mode**: Enhanced compatibility with existing desktop sessions
- **Multi-User**: Proper permission handling for different user scenarios

## 🚀 Installation & Upgrade Instructions

### Fresh Installation
```bash
git clone https://github.com/GeneticxCln/Axiom.git
cd Axiome
./install-axiom-v3.5.1.sh
```

### Upgrade from Previous Version
```bash
cd /path/to/Axiom
git pull origin main
ninja -C build
sudo ninja -C build install
```

### Post-Installation Steps
1. **Log out and back in** - Required for render group membership
2. **Select "Axiom" from SDDM** - Session should now work properly
3. **Check logs if needed** - Use `./axiom-session-debug` for troubleshooting

## 🔍 Troubleshooting

### If Session Still Fails
1. Run `./axiom-session-debug` for diagnostic information
2. Check `~/.cache/axiom/logs/session.log` for error details
3. Verify render group membership: `groups | grep render`
4. Test in nested mode first: `axiom --nested`

### Common Issues
- **"Device or resource busy"**: Another compositor is already running
- **Permission denied**: User not in video/render groups
- **Segmentation fault**: Check dependencies with debug script

### Debug Command Examples
```bash
# Comprehensive system check
./axiom-session-debug

# Test primary display mode
./test-primary-mode

# Check session logs
tail -f ~/.cache/axiom/logs/session.log
```

## 📈 Quality Assurance

### Testing Completed
- ✅ SDDM compatibility verified
- ✅ Nested mode functionality confirmed
- ✅ Session cleanup behavior validated
- ✅ Permission system tested
- ✅ All existing unit tests still pass (10/10)
- ✅ Build system integration verified

### Platforms Tested
- CachyOS Linux (primary development platform)
- Generic Arch Linux systems
- SDDM display manager environments

## 🔗 Links & Resources

- **GitHub Repository**: https://github.com/GeneticxCln/Axiom
- **Issue Tracker**: https://github.com/GeneticxCln/Axiom/issues
- **Documentation**: README.md in repository
- **Previous Release**: [v4.3.3](https://github.com/GeneticxCln/Axiom/releases/tag/v4.3.3)

## 🙏 Acknowledgments

This release addresses critical user-reported issues with session management and SDDM compatibility. Thank you to all users who provided detailed bug reports and system information that made these fixes possible.

---

**Full Changelog**: [v4.3.3...v4.3.4](https://github.com/GeneticxCln/Axiom/compare/v4.3.3...v4.3.4)
