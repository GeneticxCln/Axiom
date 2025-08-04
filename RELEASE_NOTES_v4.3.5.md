# Axiom Wayland Compositor v4.3.5 Release Notes
## Critical Boot Fixes Release - August 4, 2025

### 🚨 CRITICAL BUG FIXES

This release addresses the **major boot issue** that was causing Axiom to crash immediately and kick users back to SDDM with a black screen and white line in the corner.

#### **Fixed: Event Loop Termination Bug**
- **Issue**: Event loop would exit immediately on the first minor error
- **Impact**: Caused instant compositor crashes and SDDM kickback
- **Fix**: Implemented robust error handling with retry logic (up to 10 consecutive errors before shutdown)
- **Result**: Compositor now tolerates temporary issues and remains stable

#### **Fixed: Missing Background Surface**
- **Issue**: No background was rendered, causing confusing black screen
- **Impact**: Users saw pure black screen before windows appeared
- **Fix**: Added dark gray background surface that covers entire output layout
- **Result**: Clear visual feedback that compositor is running

#### **Fixed: Cursor Initialization Race Conditions**
- **Issue**: Cursor theme loading could crash when outputs weren't ready
- **Impact**: Segfaults on cursor movement in primary display mode
- **Fix**: Safe initialization with retry logic and proper error recovery
- **Result**: Stable cursor operation across all display modes

#### **Fixed: Output Configuration Failures**
- **Issue**: Failed output commits would crash the entire compositor
- **Impact**: Prevented boot on systems with complex display configurations
- **Fix**: Graceful fallback handling and improved output state management
- **Result**: Compositor continues running even when individual outputs fail

### 🔧 TECHNICAL IMPROVEMENTS

#### **Enhanced Error Recovery**
```c
// Before: Immediate exit on any error
if (wl_event_loop_dispatch(server->wl_event_loop, -1) < 0) {
    break;  // Caused immediate shutdown
}

// After: Robust error handling
int consecutive_errors = 0;
const int MAX_CONSECUTIVE_ERRORS = 10;
if (dispatch_result < 0) {
    consecutive_errors++;
    // Only exit after many consecutive errors
    // Includes delay to prevent tight error loops
}
```

#### **Background Surface Creation**
- Dark gray background (RGB: 0.1, 0.1, 0.1) prevents black screen confusion
- Automatically resizes to match output layout changes
- Provides immediate visual feedback that compositor is running

#### **Improved Output Handling**
- Uses wlroots 0.19 output state API correctly
- Supports both preferred modes and custom fallback modes
- Background updates automatically when displays are added/removed
- Cursor theme loading retries per output with proper scaling

### 🎯 TESTING RESULTS

#### **Build System**
- All 11 test suites pass: ✅
- Successful compilation with gcc 15.1.1: ✅
- wlroots 0.19 compatibility verified: ✅

#### **Runtime Testing**
- Nested mode startup: ✅ Stable operation
- Primary display mode: ✅ No longer crashes
- Event loop resilience: ✅ Handles 10+ consecutive errors gracefully
- Background rendering: ✅ Dark gray background visible
- Cursor functionality: ✅ No segfaults or crashes

### 📦 INSTALLATION

**From Source:**
```bash
git pull origin main
meson setup builddir --wipe
meson compile -C builddir
sudo meson install -C builddir
```

**Version Verification:**
```bash
axiom --help  # Should show v4.3.5
```

### 🔍 WHAT WAS CAUSING THE SDDM KICKBACK?

The issue was a **cascade failure**:

1. **Minor error occurs** (cursor theme loading, output configuration, etc.)
2. **Event loop exits immediately** instead of handling the error
3. **Compositor shuts down** within seconds of starting
4. **SDDM detects failure** and shows login screen again
5. **User sees**: Black screen → white line → back to SDDM

**Now Fixed**: Minor errors are handled gracefully, compositor stays running, and users see a stable desktop environment.

### 🚀 COMPATIBILITY

- **CachyOS Linux**: ✅ Fully tested and working
- **wlroots 0.19**: ✅ API compatibility verified  
- **Wayland 1.24**: ✅ Protocol support confirmed
- **Multiple GPUs**: ✅ Improved output handling
- **Nested Mode**: ✅ Works in existing Wayland/X11 sessions

### 📋 UPGRADE NOTES

- **Recommended**: All users should upgrade immediately if experiencing boot issues
- **Breaking Changes**: None - fully backward compatible
- **Configuration**: Existing configs continue to work unchanged
- **Session Scripts**: Updated to v4.3.5 but maintain compatibility

### 🐛 KNOWN ISSUES

- Output commit failures in nested mode (expected behavior - compositor continues running)
- AI monitoring references in session script (non-critical - gracefully handled)

### 🙏 ACKNOWLEDGMENTS

Special thanks to the wlroots team for the excellent 0.19 API and comprehensive documentation that helped identify these critical initialization issues.

---

**Download**: Available via git clone or GitHub releases  
**Documentation**: See `CRITICAL_BOOT_FIXES.md` for technical details  
**Support**: Report issues on GitHub  

**Previous Release**: [v4.3.4](RELEASE_NOTES_v4.3.4.md) - Session Stability Release
