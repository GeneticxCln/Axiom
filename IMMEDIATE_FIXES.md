# 🚨 IMMEDIATE FIXES REQUIRED

## CRITICAL ISSUES TO FIX TODAY

### 1. **Remove Debug Code from Production** ⚠️ URGENT

**Files with printf() debug statements:**
```bash
# Check these files immediately:
src/main.c:589-592       # Help text (can stay)
src/environment.c        # Multiple printf statements
src/logging.c           # Debug printf mixed with logging
src/keybindings.c       # Debug output
src/screenshot.c        # Debug messages
```

**Action:**
```bash
# Quick audit command:
grep -n printf src/*.c | grep -v "logging.c"
```

### 2. **Architecture Cleanup** 🏗️

**Problem:** 31 C files with unclear separation
**Target:** Reduce to ~20 files with clear responsibilities

**Files to merge/consolidate:**
- `keybindings.c` + `focus.c` → `input_manager.c`
- `window_snapping.c` + `smart_gaps.c` → `window_layout.c`
- `screenshot.c` + `session.c` → `desktop_integration.c`

### 3. **Memory Management Simplification** 💾

**Current:** 790+ lines of custom tracking
**Target:** Reduce to ~300 lines, debug-only tracking

**Files to audit:**
- `src/memory.c` - Too complex for production
- All calls to `axiom_calloc_tracked()` - Performance overhead

## QUICK WINS (1-2 hours each)

### Fix 1: Remove printf statements
```bash
# Replace printf with proper logging
sed -i 's/printf(/axiom_log_info(/g' src/environment.c
```

### Fix 2: Consolidate window management
```bash
# Merge related functionality
# Create src/window_manager_unified.c
```

### Fix 3: Simplify configuration reload
```bash
# Remove complex reload in main.c lines 33-94
# Use simpler config update mechanism
```

## VALIDATION TESTS

After each fix, run:
```bash
# Build test
meson compile -C builddir

# Integration test
./test_rendering_integration.sh

# Stress test (abbreviated)
timeout 30s ./test_rendering_stress.sh
```

## SUCCESS CRITERIA

- [ ] Zero printf statements outside help text
- [ ] Build time reduced by >20%
- [ ] Memory usage baseline <100MB
- [ ] All tests still passing

## RISK MITIGATION

**Before making changes:**
1. Create backup: `git branch backup-$(date +%Y%m%d)`
2. Document current test results
3. Make incremental changes with commits

**If things break:**
```bash
git checkout backup-$(date +%Y%m%d)
# Start with smaller changes
```

## TIME ESTIMATE

- **Debug cleanup**: 2-3 hours
- **Architecture consolidation**: 1-2 days  
- **Memory management**: 1 day
- **Testing/validation**: 4-6 hours

**Total: 3-4 days of focused work**

This will move the project from **35% production ready** to **~70% production ready**.
