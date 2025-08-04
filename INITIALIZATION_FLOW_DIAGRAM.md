# Axiom Compositor Initialization Flow Diagram

```
📋 AXIOM COMPOSITOR INITIALIZATION SEQUENCE
═══════════════════════════════════════════════════════════════

🔧 PHASE 1: PRE-OUTPUT SETUP (Lines 31-186)
┌────────────────────────────────────────────────────────────┐
│                                                            │
│  1️⃣  Basic Infrastructure                                 │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wl_display_create()                          │   │
│      │ • wl_event_loop                                │   │
│      │ • Server structure initialization             │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  2️⃣  Graphics Backend                                     │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wlr_backend_autocreate() (NOT started)       │   │
│      │ • Detects: DRM, Wayland, X11, etc.            │   │
│      │ • Status: CREATED but DORMANT                  │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  3️⃣  Rendering System                                     │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wlr_renderer_autocreate()                    │   │
│      │ • GPU context: OpenGL/Vulkan                   │   │
│      │ • wlr_allocator (GPU memory)                   │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  4️⃣  Wayland Protocols                                    │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wlr_compositor_create()                       │   │
│      │ • wlr_xdg_shell_create()                        │   │
│      │ • wlr_seat_create()                             │   │
│      │ • data_device_manager                           │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  5️⃣  Scene Management                                     │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wlr_scene_create()                            │   │
│      │ • wlr_output_layout_create() (EMPTY)            │   │
│      │ • Background rectangle (dark gray)             │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  6️⃣  Input Infrastructure                                 │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wlr_cursor_create() (unattached)              │   │
│      │ • cursor_mgr + theme loading                    │   │
│      │ • cursor_mode = PASSTHROUGH                     │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  7️⃣  Axiom Feature Systems                               │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • axiom_config_create()                         │   │
│      │ • axiom_animation_manager_init()                │   │
│      │ • axiom_effects_manager_init()                  │   │
│      │ • axiom_window_manager_create()                 │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  8️⃣  Event System                                         │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • Set up all event listeners                    │   │
│      │ • new_output, new_input, new_xdg_toplevel       │   │
│      │ • Status: ARMED and READY                       │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  9️⃣  Client Interface                                     │
│      ┌─────────────────────────────────────────────────┐   │
│      │ • wl_display_add_socket_auto()                  │   │
│      │ • Creates: wayland-0, wayland-1, etc.          │   │
│      │ • Status: READY for connections                │   │
│      └─────────────────────────────────────────────────┘   │
│                                                            │
└────────────────────────────────────────────────────────────┘

🚀 CRITICAL TRANSITION: wlr_backend_start() (Line 188)
═══════════════════════════════════════════════════════════════
│ This single call triggers ALL hardware detection events!   │
│ Everything after this point is EVENT-DRIVEN               │
╰─────────────────────────────────────────────────────────────╯
                            ↓

📱 PHASE 2: HARDWARE DETECTION (Event-Driven)
┌────────────────────────────────────────────────────────────┐
│                                                            │
│  🖥️  OUTPUT DETECTION                                     │
│      ┌─────────────────────────────────────────────────┐   │
│      │ EVENT: new_output signal fired                  │   │
│      │ ├─ axiom_new_output() called                    │   │
│      │ ├─ Output WL-1 detected (1280x720)             │   │
│      │ ├─ wlr_output_commit_state()                    │   │
│      │ ├─ Scene integration                            │   │
│      │ └─ Background resized to match                  │   │
│      └─────────────────────────────────────────────────┘   │
│                            ↓                               │
│  ⌨️  INPUT DETECTION                                       │
│      ┌─────────────────────────────────────────────────┐   │
│      │ EVENT: new_input signal fired                   │   │
│      │ ├─ axiom_new_input() called multiple times     │   │
│      │ ├─ Keyboard: wayland-keyboard-Hyprland         │   │
│      │ └─ Pointer: wayland-pointer-Hyprland           │   │
│      └─────────────────────────────────────────────────┘   │
│                                                            │
└────────────────────────────────────────────────────────────┘

🏃 PHASE 3: MAIN LOOP (Line 205)
┌────────────────────────────────────────────────────────────┐
│                                                            │
│  🔄 EVENT LOOP RUNNING                                    │
│      ┌─────────────────────────────────────────────────┐   │
│      │ while (server->running) {                       │   │
│      │   wl_display_flush_clients()                    │   │
│      │   axiom_animation_manager_update()              │   │
│      │   wl_event_loop_dispatch() // BLOCKS HERE       │   │
│      │ }                                               │   │
│      └─────────────────────────────────────────────────┘   │
│                                                            │
│  ✅ STATUS: READY FOR CLIENTS                            │
│      ┌─────────────────────────────────────────────────┐   │
│      │ Environment: WAYLAND_DISPLAY=wayland-0          │   │
│      │ Socket: /run/user/1000/wayland-0                │   │
│      │ Clients can connect and create surfaces         │   │
│      └─────────────────────────────────────────────────┘   │
│                                                            │
└────────────────────────────────────────────────────────────┘

💡 KEY INSIGHTS:
═════════════════════════════════════════════════════════════

✅ BEFORE OUTPUT:
   • All infrastructure is ready
   • All managers initialized  
   • All protocols available
   • Event system armed
   • Socket created and listening

⚡ AT BACKEND START:
   • Hardware discovery happens
   • Outputs and inputs detected
   • Scene graph populated

🔄 AFTER OUTPUT:
   • Compositor fully operational
   • Ready for client applications
   • All systems working together

🎯 ARCHITECTURE BENEFIT:
   This design ensures zero race conditions - everything
   is ready before hardware events start firing!
```
