# DWM Feature Analysis for Axiom Compositor

## Overview
This document analyzes the dwm (dynamic window manager) source code to identify key features that should be implemented in the Axiom Wayland compositor to provide a comparable or superior user experience.

## Current Axiom Status vs DWM Features

### ✅ **Already Implemented in Axiom**

1. **Basic Tiling Management**
   - ✅ Window tiling with configurable layouts
   - ✅ Multiple layout algorithms (grid, master-stack)
   - ✅ Window focus management
   - ✅ Interactive window movement and resizing

2. **Configuration System**
   - ✅ File-based configuration
   - ✅ Runtime configuration changes
   - ✅ Window rules system (more advanced than dwm)

3. **Multi-Monitor Support**
   - ✅ Multiple output support
   - ✅ Per-monitor layout management

### 🔄 **Partially Implemented - Need Enhancement**

1. **Tag System (Workspaces)**
   - ✅ Basic workspace support (9 workspaces)
   - ❌ **Missing: Tag-based window organization**
   - ❌ **Missing: Multiple tag assignment per window**
   - ❌ **Missing: Tag toggling and combinations**

2. **Master-Stack Layout**
   - ✅ Basic master-stack available
   - ❌ **Missing: Adjustable master area ratio**
   - ❌ **Missing: Dynamic master count adjustment**
   - ❌ **Missing: Stack area optimization**

3. **Window Management**
   - ✅ Basic window operations
   - ❌ **Missing: Window urgency hints**
   - ❌ **Missing: Window size hints respect**
   - ❌ **Missing: Zoom functionality (swap with master)**

### ❌ **Missing Critical Features**

## 🎯 **Priority Features to Implement**

### 1. **Enhanced Tag System** (High Priority)
```c
// DWM-style tagging we should implement:
struct axiom_window {
    // ... existing fields ...
    uint32_t tags;              // Bitmask of assigned tags
    bool is_urgent;             // Urgency hint support
};

struct axiom_workspace {
    // ... existing fields ...
    uint32_t tagset[2];         // Current and previous tag selection
    uint32_t selected_tags;     // Currently visible tags
};
```

**Key Functions Needed:**
- `axiom_tag_window(window, tag_mask)` - Assign tags to window
- `axiom_view_tags(tag_mask)` - Show windows with specific tags
- `axiom_toggle_tag(tag_mask)` - Toggle tag visibility
- `axiom_window_toggle_tag(window, tag_mask)` - Toggle window tag assignment

### 2. **Master-Stack Layout Enhancement** (High Priority)
```c
struct axiom_master_stack_config {
    float master_factor;        // Ratio of master area (0.05-0.95)
    int master_count;          // Number of windows in master area
    bool respect_size_hints;   // Honor window size constraints
};
```

**Key Functions Needed:**
- `axiom_adjust_master_factor(delta)` - Resize master area
- `axiom_adjust_master_count(delta)` - Change number of master windows
- `axiom_zoom_window()` - Swap focused window with master

### 3. **Status Bar System** (Medium Priority)
dwm has a sophisticated status bar that displays:
- Tag indicators with visual state
- Layout symbol
- Window title
- Status text from external scripts

```c
struct axiom_status_bar {
    bool enabled;
    bool top_bar;              // true = top, false = bottom
    int height;
    struct wlr_scene_tree *bar_tree;
    
    // Bar sections
    struct axiom_bar_section *tag_section;
    struct axiom_bar_section *layout_section;
    struct axiom_bar_section *title_section;
    struct axiom_bar_section *status_section;
};
```

### 4. **Advanced Window Stack Management** (Medium Priority)
```c
// Enhanced focus stack like dwm
struct axiom_focus_stack {
    struct axiom_window **stack;
    size_t count;
    size_t capacity;
};
```

**Key Functions:**
- `axiom_focus_stack_push(window)` - Add to focus history
- `axiom_focus_stack_remove(window)` - Remove from focus history
- `axiom_focus_next()` - Focus next in stack
- `axiom_focus_prev()` - Focus previous in stack

### 5. **Window Urgency System** (Low Priority)
```c
struct axiom_urgency_system {
    struct wl_list urgent_windows;
    struct wl_event_source *urgency_timer;
    bool blink_urgent;
};
```

## 🛠 **Implementation Plan**

### Phase 1: Enhanced Tagging System (Week 1-2)
1. **Tag Data Structures**
   ```c
   // Add to axiom.h
   #define AXIOM_TAG_COUNT 9
   #define AXIOM_TAG_ALL ((1 << AXIOM_TAG_COUNT) - 1)
   
   struct axiom_tag_manager {
       uint32_t current_tags;
       uint32_t previous_tags;
       char *tag_names[AXIOM_TAG_COUNT];
       bool tag_occupied[AXIOM_TAG_COUNT];
   };
   ```

2. **Core Tag Functions**
   - Implement tag assignment logic
   - Add tag visibility filtering
   - Create tag switching animations

3. **Configuration Integration**
   ```ini
   [tagging]
   tag_names=["web", "dev", "term", "media", "chat", "files", "games", "sys", "misc"]
   default_tag=1
   show_empty_tags=false
   ```

### Phase 2: Master-Stack Layout Enhancement (Week 3)
1. **Layout Algorithm Improvement**
   ```c
   void axiom_layout_master_stack_advanced(struct axiom_server *server) {
       struct axiom_master_stack_config *config = &server->layout_config.master_stack;
       
       // Calculate master area based on factor and window count
       int master_width = server->workspace_width * config->master_factor;
       int stack_width = server->workspace_width - master_width;
       
       // Distribute windows with size hint respect
       // ... implementation
   }
   ```

2. **Dynamic Master Controls**
   - Keyboard shortcuts for master area adjustment
   - Mouse-based master area resizing
   - Master count adjustment

### Phase 3: Status Bar Implementation (Week 4-5)
1. **Bar Rendering System**
   ```c
   struct axiom_status_bar *axiom_status_bar_create(struct axiom_server *server);
   void axiom_status_bar_update(struct axiom_status_bar *bar);
   void axiom_status_bar_render_tags(struct axiom_status_bar *bar);
   void axiom_status_bar_render_layout(struct axiom_status_bar *bar);
   ```

2. **External Status Integration**
   - Script-based status updates
   - System information display
   - Click handling for bar sections

### Phase 4: Focus Stack & Urgency (Week 6)
1. **Focus History**
   - Implement dwm-style focus stack
   - Add Alt+Tab style window cycling
   
2. **Urgency System**
   - X11 urgency hint support
   - Visual urgency indicators
   - Notification integration

## 📊 **Key DWM Concepts to Adopt**

### 1. **Simplicity Philosophy**
dwm's strength is its simplicity. Axiom should maintain:
- Minimal configuration complexity
- Predictable behavior
- Efficient resource usage

### 2. **Tag-Based Organization**
dwm's tagging system is more flexible than traditional workspaces:
- Windows can belong to multiple tags
- Tag combinations create dynamic workspaces
- More flexible than fixed workspace model

### 3. **Master-Stack Efficiency**
dwm's master-stack layout is highly productive:
- Main work area (master) always gets optimal space
- Secondary windows efficiently arranged in stack
- Quick master area adjustments

### 4. **Keyboard-Driven Workflow**
dwm excels at keyboard efficiency:
- Every action has a keyboard shortcut
- Minimal mouse dependency
- Consistent keybinding patterns

## 🔄 **Axiom Advantages Over DWM**

### Current Axiom Strengths:
1. **Modern Wayland Protocol Support**
2. **Advanced Visual Effects** (shadows, blur, animations)
3. **Sophisticated Window Rules System**
4. **Real-time Effects and GPU Acceleration**
5. **Advanced Configuration System**
6. **XWayland Integration**

### Features Axiom Should Keep:
- Keep all existing advanced features
- Don't sacrifice Wayland benefits for X11 compatibility
- Maintain modern compositor capabilities

## 📋 **Action Items**

### Immediate (Next Sprint):
1. ✅ Complete this analysis
2. 🔄 **Implement enhanced tag system**
3. 🔄 **Add master area ratio adjustment**
4. 🔄 **Create zoom functionality**

### Short Term (Next Month):
1. 🔄 **Status bar implementation**
2. 🔄 **Focus stack enhancement**
3. 🔄 **Urgency system**
4. 🔄 **Size hints respect**

### Long Term (Next Quarter):
1. 🔄 **Advanced keyboard shortcuts**
2. 🔄 **External status integration**
3. 🔄 **Layout switching animations**
4. 🔄 **Multi-monitor tag support**

## 💡 **Conclusion**

dwm provides an excellent reference for efficient tiling window management. The key features to implement are:

1. **Enhanced tagging system** - More flexible than current workspaces
2. **Master-stack improvements** - Dynamic ratio and count adjustment
3. **Status bar** - Essential for productivity workflows
4. **Focus stack** - Better window navigation
5. **Urgency system** - Notification integration

By implementing these features while maintaining Axiom's modern Wayland capabilities and visual effects, we can create a compositor that combines dwm's efficiency with modern functionality.
