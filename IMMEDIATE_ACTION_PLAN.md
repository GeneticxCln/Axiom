# 🚨 IMMEDIATE ACTION PLAN - Make Axiom Functional

## **Current Critical Issues Found:**
- **100+ printf() debug statements** scattered across codebase
- **10,138 lines of C code** in a monolithic structure
- **Basic architecture** but missing professional practices
- **Week 1 fixes completed** but major work still needed

---

## **🎯 PHASE 1: CRITICAL FIXES (Week 1-2)**

### **Priority 1: Replace All Debug Printf Statements**
**Impact**: Professional logging, easier debugging, better performance

```bash
# Files with most printf() issues:
src/main.c           - 50+ printf statements
src/logging.c        - 20+ printf statements  
src/renderer.c       - 12+ printf statements
src/window_snapping.c - 13+ printf statements
```

**Action Plan:**
1. Create centralized logging macros
2. Replace all printf() with proper log levels
3. Add log filtering and output control

### **Priority 2: Fix Memory Management**
**Files to audit:**
- `src/main.c` - 900+ lines, complex initialization
- `src/thumbnail_manager.c` - Already partially fixed
- `src/window_snapping.c` - Complex state management

### **Priority 3: Build System Professional Standards**
**Current Issues:**
- Build works but has warnings
- No proper installation/uninstall
- Missing dependency validation

---

## **🏗️ PHASE 2: CORE FUNCTIONALITY (Week 3-6)**

### **Window Management Issues to Fix:**

#### **2.1 Focus System (BROKEN)**
```bash
# Test this - likely broken:
./axiom --nested
# Try Alt+Tab - probably doesn't work
# Try clicking between windows - focus issues
```

#### **2.2 Keyboard Shortcuts (UNRELIABLE)**
```bash
# Test current shortcuts:
Super+Q     # Quit (may work)
Super+T     # New terminal (likely broken)  
Super+1-9   # Workspace switching (fake)
Super+Arrow # Window management (buggy)
```

#### **2.3 Mouse Interaction (BUGGY)**
```bash
# Test these - likely issues:
# - Window dragging
# - Window resizing
# - Right-click menus
# - Cursor themes
```

---

## **🧪 PHASE 3: TESTING REALITY CHECK (Week 7-8)**

### **Current Test Status:**
```bash
meson test -C builddir
# Shows: 8/8 tests pass
# Reality: These are trivial placeholder tests
```

### **What's Actually Needed:**
- **Integration tests**: Full compositor startup/shutdown
- **Window management tests**: Multi-window scenarios  
- **Performance tests**: Memory usage, frame rates
- **Stress tests**: 50+ windows, rapid creation/destruction
- **Multi-monitor tests**: Display configuration changes

---

## **📊 REALISTIC TIMELINE TO "FUNCTIONAL"**

### **Minimum Viable Compositor (8-10 weeks):**

**Week 1-2: Foundation**
- ✅ Fix compiler warnings (DONE)
- [ ] Replace all printf() with logging
- [ ] Fix major memory leaks
- [ ] Professional build system

**Week 3-4: Basic Window Management**
- [ ] Fix window positioning bugs
- [ ] Implement reliable focus system
- [ ] Basic keyboard shortcuts that work
- [ ] Mouse interaction that works

**Week 5-6: Essential Features**
- [ ] Multi-monitor support that works
- [ ] Configuration system that loads properly
- [ ] Workspace switching (real, not fake)
- [ ] Basic window decorations

**Week 7-8: Polish & Testing**
- [ ] Comprehensive test suite
- [ ] Performance optimization
- [ ] Memory leak elimination
- [ ] Documentation updates

**Week 9-10: Stabilization**
- [ ] Bug fixes from testing
- [ ] Performance tuning
- [ ] User experience improvements
- [ ] Release preparation

---

## **💡 QUICK WINS TO START WITH**

### **This Weekend (2-3 hours):**
1. **Create proper logging system** - Replace top 20 printf() calls
2. **Fix one major bug** - Pick window positioning or focus
3. **Add error handling** - Stop silent failures in main.c

### **Next Week (10-15 hours):**
1. **Clean up main.c** - Split into logical modules
2. **Fix keyboard shortcuts** - Make Super+Q, Super+T actually work
3. **Add basic tests** - Real window management tests

### **Following Week (15-20 hours):**
1. **Multi-monitor fixes** - Test with actual multi-monitor setup
2. **Mouse interaction** - Fix window dragging and resizing
3. **Performance profiling** - Find and fix major bottlenecks

---

## **🛠️ DEVELOPMENT SETUP RECOMMENDATIONS**

### **Tools You'll Need:**
```bash
# Install debugging tools:
sudo pacman -S gdb valgrind strace

# Install development tools:
sudo pacman -S clang-tools-extra  # For static analysis
sudo pacman -S perf               # For performance profiling

# Install Wayland debugging:
sudo pacman -S wayland-utils      # wayland-info, etc.
```

### **Testing Setup:**
```bash
# Create nested session for testing:
./axiom --nested   # Test in existing compositor

# Monitor resource usage:
htop               # Watch memory/CPU usage
journalctl -f      # Watch system logs

# Profile performance:
perf record ./axiom --nested
perf report        # Analyze performance
```

---

## **🎯 SUCCESS CRITERIA FOR "FUNCTIONAL"**

### **After 8-10 weeks, Axiom should:**
- [ ] **Start reliably** without crashes
- [ ] **Manage windows** correctly (positioning, focus, decorations)
- [ ] **Handle input** properly (keyboard shortcuts, mouse interaction)
- [ ] **Support multi-monitor** configurations
- [ ] **Load configuration** without errors
- [ ] **Run for hours** without memory leaks
- [ ] **Pass comprehensive tests** (50+ real test cases)
- [ ] **Perform acceptably** (60fps, <200MB RAM baseline)

### **User Experience Test:**
*"Can a Linux user switch from i3/Hyprland to Axiom and have a reasonable daily-driver experience?"*

**Currently**: No - too many bugs and missing features
**After 8-10 weeks**: Yes - basic but functional compositor

---

## **💰 RESOURCE REQUIREMENTS**

### **For Individual Developer:**
- **Time**: 15-25 hours/week for 8-10 weeks
- **Skills**: Strong C programming, Wayland protocol knowledge
- **Hardware**: Multi-monitor setup for testing
- **Focus**: Dedicated time blocks, not scattered hours

### **For Team Approach:**
- **2 developers**: 6-8 weeks total time
- **3 developers**: 4-6 weeks total time
- **Skills needed**: C/Wayland expert + 1-2 general developers

---

## **🚀 GET STARTED TODAY**

### **Hour 1: Assessment**
```bash
# Run Axiom and document what's broken:
./axiom --nested 2>&1 | tee axiom-issues.log
# Try basic operations, note failures
```

### **Hour 2: Quick Win**
```bash
# Pick ONE printf-heavy file and fix it:
# Good candidate: src/renderer.c (12 printf statements)
# Replace with proper logging macros
```

### **Hour 3: Plan Next Steps**
```bash
# Choose your approach:
# A) Systematic (follow this roadmap)
# B) Quick fixes (tackle biggest issues first)  
# C) Rewrite (start over with lessons learned)
```

---

**Bottom Line**: Axiom needs 8-10 weeks of focused development to become truly functional. The foundation is there, but professional development practices must be applied systematically.

**Start today with logging fixes - they're high-impact and will make all other debugging easier!**
