# Axiom v4.3.3 - Critical Bug Fix Release

**Release Date**: August 3, 2025  
**Type**: Bug Fix Release  
**Priority**: Critical  

## 🐛 Critical Bug Fixes

### Segmentation Fault Fix
- **Fixed**: Critical segmentation fault during compositor initialization
- **Root Cause**: Uninitialized `input_devices` linked list in server structure
- **Impact**: Compositor would crash immediately when starting in nested mode
- **Solution**: Added proper `wl_list_init(&server->input_devices)` initialization

## 🔧 Technical Details

**Files Changed:**
- `src/compositor.c` - Added missing list initialization

**Code Change:**
```c
// Initialize window lists
wl_list_init(&server->windows);
wl_list_init(&server->outputs);
+wl_list_init(&server->input_devices);  // <- Critical fix
```

## ✅ Testing

- ✅ Compositor starts successfully in nested mode
- ✅ Keyboard input devices initialize properly
- ✅ No more segmentation faults on startup
- ✅ All existing functionality preserved

## 🚀 Installation

### From Source
```bash
git clone https://github.com/GeneticxCln/Axiom.git
cd Axiom
git checkout v4.3.3
meson setup builddir
cd builddir
ninja
```

### Running
```bash
# Nested mode (for testing)
./builddir/axiom --nested

# Native mode (as display manager)
./builddir/axiom
```

## 🔗 Links

- **Repository**: https://github.com/GeneticxCln/Axiom
- **Issues**: https://github.com/GeneticxCln/Axiom/issues
- **Documentation**: See README.md

## 📋 What's Next

This is a critical bug fix release. Users experiencing startup crashes should upgrade immediately.

**Upcoming Features:**
- Enhanced window management
- Improved animation system
- Better error handling
- Performance optimizations

---

**Full Changelog**: https://github.com/GeneticxCln/Axiom/compare/v4.3.2...v4.3.3
