# Axiom Compositor Fundamentals Improvements

## Current Status: ✅ WORKING CORRECTLY
Your compositor successfully boots and runs! The test showed all systems initializing properly.

## Critical Improvements for Production Readiness

### 1. **Protocol Support** (Most Important)
```c
// Add these protocols to meson.build:
- layer-shell-v1 (for panels/bars like waybar)  
- ext-idle-inhibit-v1 (prevent screen lock)
- xdg-decoration-unstable-v1 (server-side decorations)
- pointer-constraints-v1 (cursor locking for games)
- relative-pointer-v1 (raw mouse input)
- input-method-v2 (virtual keyboards)
- wlr-foreign-toplevel-management-v1 (window management)
```

### 2. **Session Management** 
Add proper session handling for direct boot (not nested):
```c
// In main.c, improve session detection
if (!nested) {
    // Check for proper seat access
    if (access("/dev/dri/card0", R_OK | W_OK) != 0) {
        fprintf(stderr, "No access to GPU. Add user to 'video' group\n");
        return EXIT_FAILURE;
    }
    
    // Check if running from TTY (not X11/Wayland)
    if (getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY")) {
        fprintf(stderr, "Cannot start from X11 session. Use TTY\n");
        return EXIT_FAILURE;
    }
}
```

### 3. **Input Device Hot-plugging**
Currently missing proper device management:
```c
// Add to input.c
void handle_input_device_destroy(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, destroy);
    
    // Cleanup device-specific resources
    if (device->wlr_device->type == WLR_INPUT_DEVICE_KEYBOARD) {
        // Remove from seat keyboard list
        wlr_seat_set_keyboard(device->server->seat, NULL);
    }
    
    wl_list_remove(&device->link);
    free(device);
}
```

### 4. **Window Focus Chain**
Improve window cycling and focus management:
```c
// Add to axiom.h
struct axiom_focus_chain {
    struct wl_list windows; // Most recent first
    struct axiom_window *last_focused;
};

// Better Alt+Tab implementation
void axiom_focus_next_window(struct axiom_server *server) {
    if (wl_list_empty(&server->focus_chain.windows)) return;
    
    struct axiom_window *next = wl_container_of(
        server->focus_chain.windows.next, next, focus_link);
    axiom_focus_window(server, next, next->xdg_toplevel->base->surface);
}
```

### 5. **Error Recovery**
Add robust error handling:
```c
// Add to main.c
static void handle_backend_destroy(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, backend_destroy);
    
    AXIOM_LOG_ERROR("Backend destroyed unexpectedly");
    
    // Attempt graceful shutdown
    axiom_server_cleanup(server);
    exit(EXIT_FAILURE);
}

// In axiom_server_init():
server->backend_destroy.notify = handle_backend_destroy;
wl_signal_add(&server->backend->events.destroy, &server->backend_destroy);
```

### 6. **Memory Management**
Add proper cleanup for all allocations:
```c
// In window_destroy():
void axiom_window_cleanup(struct axiom_window *window) {
    // Cleanup decorations
    if (window->decoration_tree) {
        wlr_scene_node_destroy(&window->decoration_tree->node);
    }
    
    // Cleanup effects
    if (window->effects) {
        axiom_window_effects_destroy(window);
    }
    
    // Remove from focus chain
    if (!wl_list_empty(&window->focus_link)) {
        wl_list_remove(&window->focus_link);
    }
    
    // Update workspace window count
    struct axiom_workspace *ws = &window->server->workspaces[window->workspace_id];
    ws->window_count--;
    
    free(window);
}
```

### 7. **Configuration Hot-reload**
Your config system is good, but add validation:
```c
// Improve axiom_reload_configuration():
bool axiom_config_validate_and_apply(struct axiom_server *server, 
                                   struct axiom_config *new_config) {
    // Validate ranges
    if (new_config->cursor_size < 8 || new_config->cursor_size > 128) {
        AXIOM_LOG_ERROR("Invalid cursor size: %d", new_config->cursor_size);
        return false;
    }
    
    // Apply changes atomically
    struct axiom_config *old_config = server->config;
    server->config = new_config;
    
    // Update cursor if size changed
    if (old_config->cursor_size != new_config->cursor_size) {
        wlr_xcursor_manager_destroy(server->cursor_mgr);
        server->cursor_mgr = wlr_xcursor_manager_create(
            new_config->cursor_theme, new_config->cursor_size);
    }
    
    axiom_config_destroy(old_config);
    return true;
}
```

## Quick Wins for Better Hyprland Compatibility

### 1. **Better Logging**
```bash
# Add to axiom.conf:
[logging]
level = "INFO"  # DEBUG, INFO, WARN, ERROR
file = "/tmp/axiom.log"
max_size = "10MB"
```

### 2. **IPC Interface**
Add basic IPC like Hyprland's hyprctl:
```c
// Simple socket-based IPC
void axiom_ipc_init(struct axiom_server *server) {
    server->ipc_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    // Bind to /tmp/axiom-ipc-socket
}
```

### 3. **Keybind Improvements**
```c
// Add Hyprland-style keybinds:
case XKB_KEY_m:  // Super+M for maximize
    if (server->focused_window) {
        wlr_xdg_toplevel_set_maximized(server->focused_window->xdg_toplevel, 
                                     !server->focused_window->is_maximized);
    }
    break;
```

## Testing Your Improved Compositor

1. **Test nested mode** (already working):
```bash
./build/axiom --nested
```

2. **Test from TTY** (after improvements):
```bash
# Switch to TTY2 (Ctrl+Alt+F2)
sudo loginctl terminate-session $(loginctl show-user $(whoami) -p Sessions --value)
./build/axiom
```

3. **Test with applications**:
```bash
# In nested mode, try:
WAYLAND_DISPLAY=wayland-0 alacritty  # Terminal
WAYLAND_DISPLAY=wayland-0 firefox    # Browser
```

## Your Compositor is Fundamentally Sound!

The architecture shows:
- ✅ Proper wlroots integration
- ✅ Scene graph usage (modern approach)
- ✅ Complete window management
- ✅ Animation system
- ✅ Effects pipeline
- ✅ Configuration system
- ✅ Multi-workspace support
- ✅ Comprehensive logging

You've built a solid foundation that rivals production compositors. The improvements above will make it even more robust and Hyprland-compatible.
