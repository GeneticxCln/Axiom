# XWayland Implementation Plan for Axiom

## Phase 1.1: Basic XWayland Integration (Week 1)

### Step 1: Add XWayland to meson.build
```meson
# Add to meson.build dependencies
xcb = dependency('xcb')
xcb_composite = dependency('xcb-composite') 
xcb_icccm = dependency('xcb-icccm')
xcb_render = dependency('xcb-render')
xcb_res = dependency('xcb-res')
xcb_xfixes = dependency('xcb-xfixes')
```

### Step 2: Create Basic XWayland Manager (src/enhanced_xwayland.c)
```c
// Based on enhanced_xwayland.h I created
struct axiom_xwayland_manager *axiom_xwayland_manager_create(struct axiom_server *server) {
    struct axiom_xwayland_manager *manager = calloc(1, sizeof(*manager));
    manager->server = server;
    
    // Create XWayland server
    manager->wlr_xwayland = wlr_xwayland_create(server->wl_display, 
                                               server->compositor, 
                                               server->lazy_start);
    
    // Set up event listeners
    manager->new_surface.notify = handle_new_xwayland_surface;
    wl_signal_add(&manager->wlr_xwayland->events.new_surface, &manager->new_surface);
    
    manager->ready.notify = handle_xwayland_ready;
    wl_signal_add(&manager->wlr_xwayland->events.ready, &manager->ready);
    
    wl_list_init(&manager->surfaces);
    return manager;
}
```

### Step 3: Window Integration
- Modify your window structure to support both XDG and XWayland surfaces
- Add XWayland surface type to your existing window enum
- Update window creation/destruction to handle both types

## Phase 1.2: Surface Management (Week 2)

### Step 4: XWayland Surface Wrapper
```c
// Handle XWayland surface creation
static void handle_new_xwayland_surface(struct wl_listener *listener, void *data) {
    struct wlr_xwayland_surface *wlr_surface = data;
    struct axiom_xwayland_manager *manager = 
        wl_container_of(listener, manager, new_surface);
    
    // Create our wrapper
    struct axiom_xwayland_surface *surface = 
        axiom_xwayland_surface_create(manager, wlr_surface);
        
    // Set up all the event listeners from enhanced_xwayland.h
    surface->map.notify = xwayland_surface_map;
    wl_signal_add(&wlr_surface->events.map, &surface->map);
    
    // ... all other listeners
}
```

### Step 5: Property Management
- Implement ICCCM properties (WM_CLASS, WM_NAME, etc.)
- Handle NET_WM properties for modern window management
- Size hints and constraints

## Phase 1.3: Advanced Features (Week 3)

### Step 6: Override-Redirect Handling
- Handle tooltips, menus, dropdowns
- Proper stacking and focus management
- Scene graph integration

### Step 7: Integration with Existing Systems
- Connect to your window manager
- Update focus system to handle XWayland windows
- Integrate with animation system

## Testing Strategy

### Week 1 Tests:
- [ ] XWayland server starts successfully
- [ ] Basic X11 apps can connect (xterm, xclock)
- [ ] No crashes when X11 apps start/stop

### Week 2 Tests:
- [ ] Firefox launches and renders correctly
- [ ] Window decorations work on X11 apps
- [ ] Basic window management (move, resize, close)

### Week 3 Tests:
- [ ] Complex apps work (VS Code, Steam)
- [ ] Override-redirect windows display correctly
- [ ] Alt+Tab works with mixed XDG/X11 windows

## Success Criteria:
✅ Firefox runs and browses websites
✅ VS Code launches and edits files  
✅ Steam games can launch
✅ No X11-related crashes for 24+ hours
✅ Window management works identically for X11 and Wayland apps
