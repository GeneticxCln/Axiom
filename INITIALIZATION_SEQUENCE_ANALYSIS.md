# Axiom Compositor Initialization Sequence Analysis

## Overview
This document analyzes what components are initialized **before** display/output starts in the Axiom compositor, based on the current codebase and runtime behavior.

## Initialization Timeline

### Phase 1: Core System Setup (Before Backend Start)

#### 1.1 Basic Wayland Infrastructure
```c
// Lines 40-46
server->wl_display = wl_display_create();           // ✅ FIRST
server->wl_event_loop = wl_display_get_event_loop(server->wl_display);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Creates the fundamental Wayland display server and event loop

#### 1.2 Backend Creation
```c
// Lines 48-61  
server->backend = wlr_backend_autocreate(server->wl_event_loop, NULL);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Creates the graphics backend (DRM, Wayland nested, X11, etc.)  
**Note**: Backend is created but NOT started yet

#### 1.3 Renderer System
```c
// Lines 63-75
server->renderer = wlr_renderer_autocreate(server->backend);
wlr_renderer_init_wl_display(server->renderer, server->wl_display);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Graphics rendering system (OpenGL, Vulkan, etc.)

#### 1.4 Memory Allocator
```c
// Line 78
server->allocator = wlr_allocator_autocreate(server->backend, server->renderer);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: GPU memory management for textures and buffers

#### 1.5 Compositor Protocol
```c
// Line 79
server->compositor = wlr_compositor_create(server->wl_display, 5, server->renderer);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Core Wayland compositor protocol handler

#### 1.6 Scene Graph System
```c
// Lines 80-82
server->scene = wlr_scene_create();
server->output_layout = wlr_output_layout_create(server->wl_display);
server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Scene management and output layout coordination

### Phase 1.7 Data Structures
```c
// Lines 84-87
wl_list_init(&server->windows);      // Window list
wl_list_init(&server->outputs);      // Output list  
wl_list_init(&server->input_devices); // Input device list
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Core data structure initialization

### Phase 1.8 Protocol Handlers
```c
// Lines 89-98
server->xdg_shell = wlr_xdg_shell_create(server->wl_display, 3);
server->seat = wlr_seat_create(server->wl_display, "seat0");
server->data_device_manager = wlr_data_device_manager_create(server->wl_display);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Window management, input handling, clipboard support

### Phase 1.9 Cursor System (Basic)
```c
// Lines 100-107
server->cursor = wlr_cursor_create();
server->cursor_mode = AXIOM_CURSOR_PASSTHROUGH;
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Cursor object created (but NOT attached to avoid assertion)

### Phase 1.10 Axiom-Specific Managers
```c
// Lines 109-136
server->config = axiom_config_create();                    // Configuration
axiom_animation_manager_init(server);                      // Animations
server->effects_manager = /* effects initialization */;    // Visual effects  
server->window_manager = axiom_window_manager_create(server); // Window management
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Axiom's high-level feature systems

### Phase 1.11 Event Listeners Setup
```c
// Lines 138-161
server->new_output.notify = axiom_new_output;
wl_signal_add(&server->backend->events.new_output, &server->new_output);
// + other event listeners
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Event handling infrastructure ready to receive events

### Phase 1.12 Scene Background
```c
// Lines 163-168
server->background = wlr_scene_rect_create(&server->scene->tree, 0, 0, 
                                          (float[]){0.1, 0.1, 0.1, 1.0});
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Dark gray background to prevent black screen

### Phase 1.13 Cursor Theme
```c
// Lines 170-178
server->cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
wlr_xcursor_manager_load(server->cursor_mgr, 1);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Cursor theme loaded (but cursor not attached yet)

### Phase 1.14 Wayland Socket
```c
// Lines 180-185
const char *socket = wl_display_add_socket_auto(server->wl_display);
```
**Status**: ✅ **Initialized before output**  
**Purpose**: Creates the Wayland socket (e.g., wayland-0) for client connections

---

## Phase 2: Backend Start (Triggers Output Detection)

### 2.1 Backend Activation
```c
// Lines 187-191
if (!wlr_backend_start(server->backend)) {
    AXIOM_LOG_ERROR("Failed to start backend");
    return false;
}
```
**Status**: 🚀 **CRITICAL TRIGGER POINT**  
**Purpose**: This is where outputs are detected and `axiom_new_output()` is called

### 2.2 Environment Setup
```c
// Lines 193-194
setenv("WAYLAND_DISPLAY", socket, true);
```
**Status**: ✅ **After backend start**  
**Purpose**: Sets environment variable for client discovery

---

## Phase 3: Output Detection (Triggered by Backend Start)

### 3.1 Output Event Triggered
When `wlr_backend_start()` is called, it triggers:
```
[INFO] New output: WL-1 (w=1280, h=720, refresh=0mHz, scale=1.00)
```

### 3.2 Output Configuration
```c
// In axiom_new_output() function
struct wlr_output_state state;
wlr_output_state_init(&state);
wlr_output_state_set_enabled(&state, true);
// ... mode configuration ...
wlr_output_commit_state(wlr_output, &state);
```

### 3.3 Scene Integration
```c
struct wlr_output_layout_output *lo = wlr_output_layout_add_auto(server->output_layout, wlr_output);
struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
```

---

## Phase 4: Input Device Detection

After output, input devices are detected:
```
[INFO] New keyboard: wayland-keyboard-Hyprland
[INFO] New pointer: wayland-pointer-Hyprland
```

---

## Summary: What's Ready Before Output?

### ✅ Fully Initialized Before Output:
1. **Wayland Display & Event Loop**
2. **Backend (created, not started)**
3. **Renderer & Allocator**
4. **Scene Graph System**
5. **All Protocol Handlers** (XDG shell, seat, data device)
6. **Axiom Managers** (animation, effects, window, config)
7. **Event Listeners** (ready to receive events)
8. **Background Surface**
9. **Cursor Object** (created but not attached)
10. **Cursor Theme** (loaded)
11. **Wayland Socket** (ready for clients)

### ⚠️ Partially Initialized:
1. **Cursor System** - Created but not attached (for stability)
2. **Output Layout** - Created but empty until first output

### ❌ Not Yet Available:
1. **Actual Outputs** - Detected only after backend start
2. **Input Devices** - Detected only after backend start
3. **Cursor Attachment** - Intentionally disabled for stability

## Key Insight

The **critical transition point** is `wlr_backend_start()` on line 188. Everything before this is setup/preparation. Everything after this is reactive to hardware detection events.

This architecture ensures all systems are ready to handle outputs and input devices as soon as they're detected by the backend.
