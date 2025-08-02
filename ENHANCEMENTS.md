# Axiom Compositor Enhancements

This document outlines the major enhancements implemented to make Axiom a more powerful and flexible Wayland compositor, inspired by dwm's philosophy and features.

## Overview of Enhancements

### 1. Enhanced Tagging System 🏷️

**Files:** `include/tagging.h`, `src/tagging.c`

**Features:**
- **Bitmask-based tagging** similar to dwm - windows can belong to multiple tags simultaneously
- **Tag viewing combinations** - view multiple tags at once (e.g., view tags 1 and 3 together)
- **Sticky windows** - windows that appear on all tags
- **Tag utilities** - tag naming, window counting, visibility management
- **Seamless integration** with existing workspace system

**Key Functions:**
- `axiom_tag_view()` - View specific tag combinations
- `axiom_tag_toggle_view()` - Toggle viewing a tag
- `axiom_window_set_tags()` - Assign tags to windows
- `axiom_window_toggle_sticky()` - Make windows sticky across all tags

### 2. Configurable Key Bindings System ⌨️

**Files:** `include/keybindings.h`, `src/keybindings.c`

**Features:**
- **Flexible key binding system** - supports custom modifier combinations
- **Action-based bindings** - extensive set of predefined actions
- **Macro support** - create complex sequences of actions
- **Runtime configuration** - add, remove, enable/disable bindings dynamically
- **Default bindings** included for immediate usability

**Key Features:**
- Support for Super, Ctrl, Alt, Shift modifiers
- Tag management keybindings (Super+1-9, Super+Shift+1-9, etc.)
- Window management (Super+w close, Super+f fullscreen, etc.)
- Layout controls (Super+l cycle layouts, Super+h/j adjust master ratio)
- System controls (Super+q quit, Super+r reload config)

### 3. Focus and Stacking Management 🎯

**Files:** `include/focus.h`, `src/focus.c`

**Features:**
- **Focus stack management** - maintains history of focused windows
- **Urgency system** - windows can be marked as urgent for attention
- **Alt+Tab style cycling** - smooth window cycling with visual feedback
- **Focus history** - remember and return to previously focused windows
- **Smart focus handling** - automatic focus management on window map/unmap

**Key Functions:**
- `axiom_focus_window()` - Enhanced focus with stack management
- `axiom_window_set_urgent()` - Mark windows as urgent
- `axiom_focus_urgent_window()` - Jump to urgent windows
- `axiom_focus_cycle_start()` - Begin Alt+Tab cycling

### 4. System Integration

**Enhanced axiom.h:**
- Added new system managers to `axiom_server` struct
- Extended `axiom_window` struct with tagging properties
- Forward declarations for all new systems

**Enhanced input.c:**
- Integrated new keybinding system with existing input handling
- Prioritizes new keybinding system over legacy handlers
- Maintains backward compatibility

## Key Benefits

### 🚀 **Productivity Enhancements**
- **Multi-tag workflows**: Organize work across multiple tag combinations
- **Customizable shortcuts**: Adapt the compositor to your workflow
- **Smart focus management**: Never lose track of important windows
- **Urgency indicators**: Get notified when windows need attention

### 🎛️ **Flexibility & Customization**
- **User-defined keybindings**: Create your own shortcuts and macros
- **Tag-based organization**: More flexible than traditional workspaces
- **Configurable behavior**: Adapt focus and stacking to your preferences
- **Runtime reconfiguration**: Change settings without restart

### 💡 **dwm-Inspired Philosophy**
- **Minimalist approach**: Core functionality without bloat
- **Keyboard-driven**: Efficient navigation without mouse dependency
- **Power user focused**: Advanced features for productivity
- **Battle-tested concepts**: Based on 15+ years of dwm evolution

## Usage Examples

### Tag Management
```bash
# View tag 1
Super+1

# Move window to tag 2
Super+Shift+2

# View tags 1 and 3 simultaneously
Super+1, then Super+Ctrl+3

# Toggle window between tags 1 and 2
Super+Ctrl+Shift+2

# Make window sticky (visible on all tags)
Super+s
```

### Advanced Window Management
```bash
# Focus urgent window
Super+u (can be configured)

# Cycle through recent windows
Alt+Tab

# Toggle window floating
Super+Space

# Adjust master ratio
Super+h (decrease), Super+j (increase)
```

### Custom Keybindings (Programmatic)
```c
// Add custom binding for launching browser
axiom_keybinding_add(manager, AXIOM_MOD_SUPER, XKB_KEY_b,
                    AXIOM_ACTION_COMMAND, 0, "firefox", "Launch browser");

// Create macro for development workflow
struct axiom_macro_step dev_macro[] = {
    {AXIOM_ACTION_TAG_VIEW, 2, ""},  // Switch to tag 2
    {AXIOM_ACTION_COMMAND, 0, "code"}, // Launch VS Code
    {AXIOM_ACTION_COMMAND, 0, "foot"}, // Launch terminal
};
axiom_keybinding_add_macro(manager, AXIOM_MOD_SUPER, XKB_KEY_F1,
                          dev_macro, 3, "Development workflow");
```

## Architecture Notes

### Design Principles
1. **Modular design**: Each system is self-contained and can be used independently
2. **Minimal overhead**: Efficient data structures and algorithms
3. **Extensible**: Easy to add new actions and behaviors
4. **Backward compatible**: Existing functionality remains unchanged

### Memory Management
- Efficient bitmask operations for tag management
- Limited history sizes to prevent memory leaks
- Proper cleanup functions for all systems
- Static buffers for frequently accessed strings

### Performance Considerations
- O(1) tag operations using bitmasks
- Cached window counts for efficient display updates
- Lazy evaluation of expensive operations
- Minimal redraws and re-arrangements

## Future Extensions

These systems provide a solid foundation for additional features:

- **Status bar integration**: Display tag states and urgent windows
- **Configuration file support**: Load/save keybindings and settings
- **IPC interface**: External control and scripting
- **Theme system**: Customizable visual indicators for focus and urgency
- **Multi-monitor tag management**: Per-monitor tag sets
- **Advanced focus modes**: Follow-focus, focus-on-hover options

## Conclusion

These enhancements transform Axiom from a basic tiling compositor into a powerful, flexible window manager that combines the best aspects of modern Wayland compositors with the proven concepts from dwm. The result is a system that's both approachable for newcomers and powerful enough for advanced users who demand maximum productivity and customization.
