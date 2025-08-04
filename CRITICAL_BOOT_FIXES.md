# Critical Boot Issues - Black Screen & SDDM Kickback Analysis

## Root Cause Identified

The "black screen with white line in corner then kicks back to SDDM" is a classic Wayland compositor crash pattern. I've identified several critical issues in your code:

## 🚨 **CRITICAL ISSUE #1**: Event Loop Termination

**Problem**: Your main event loop immediately exits on the first error:

```c
// In src/compositor.c line 223
if (wl_event_loop_dispatch(server->wl_event_loop, -1) < 0) {
    break;  // This immediately exits the compositor!
}
```

**Impact**: Any minor error (missing cursor theme, protocol issue, etc.) causes immediate shutdown.

**Fix**: Add proper error handling:

```c
if (wl_event_loop_dispatch(server->wl_event_loop, -1) < 0) {
    AXIOM_LOG_ERROR("Event loop dispatch failed, but continuing...");
    // Don't break immediately - only break on critical errors
    // Check if display is still valid
    if (!server->wl_display) {
        AXIOM_LOG_ERROR("Display destroyed, shutting down");
        break;
    }
    // Add small delay to prevent tight error loop
    usleep(1000);
}
```

## 🚨 **CRITICAL ISSUE #2**: Cursor Manager Crashes

**Problem**: In `src/cursor.c`, cursor theme loading can crash on primary display:

```c
// Line 42 - This can segfault if no outputs are ready
if (wlr_xcursor_manager_load(server->cursor_mgr, 1.0)) {
    cursor_theme_loaded = true;
}
```

**Impact**: Immediate crash when cursor is first moved.

**Fix**: Add proper initialization order and safety checks.

## 🚨 **CRITICAL ISSUE #3**: Missing Background Surface

**Problem**: No background is drawn, causing the "black screen" effect. The server structure declares:

```c
struct wlr_scene_rect *background;  // Background to prevent black screen
```

But this is never actually created or rendered!

**Impact**: Black screen visible before any windows are created.

## 🚨 **CRITICAL ISSUE #4**: XWayland Initialization Race

**Problem**: XWayland is started immediately but may not be ready when clients try to connect.

```c
// In xwayland.c - no proper ready state handling
manager->lazy = false;    // Start immediately, not on demand
```

**Impact**: X11 apps crash the compositor or fail to start.

## Complete Fix Implementation

Here are the exact fixes needed:

### Fix 1: Robust Event Loop

```c
// Replace the event loop in axiom_compositor_run()
void axiom_compositor_run(struct axiom_server *server) {
    if (!server) return;

    server->running = true;
    AXIOM_LOG_INFO("Axiom running on Wayland display");
    
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 10;

    while (server->running) {
        wl_display_flush_clients(server->wl_display);

        // Update animations if the animation manager is available
        if (server->animation_manager) {
            uint32_t current_time = get_current_time_ms();
            axiom_animation_manager_update(server->animation_manager, current_time);
        }

        int dispatch_result = wl_event_loop_dispatch(server->wl_event_loop, -1);
        if (dispatch_result < 0) {
            consecutive_errors++;
            AXIOM_LOG_ERROR("Event loop dispatch failed (attempt %d/%d)", 
                           consecutive_errors, MAX_CONSECUTIVE_ERRORS);
            
            // Only exit if we have too many consecutive errors
            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                AXIOM_LOG_ERROR("Too many consecutive errors, shutting down");
                break;
            }
            
            // Check if display is still valid
            if (!server->wl_display) {
                AXIOM_LOG_ERROR("Display destroyed, shutting down");
                break;
            }
            
            // Small delay to prevent tight error loop
            usleep(10000); // 10ms
        } else {
            // Reset error counter on successful dispatch
            consecutive_errors = 0;
        }
    }
}
```

### Fix 2: Safe Cursor Initialization

```c
// Add to axiom_compositor_init() after cursor creation
static void setup_background_and_cursor(struct axiom_server *server) {
    // Create a background surface to prevent black screen
    server->background = wlr_scene_rect_create(&server->scene->tree, 0, 0, 
                                              (float[]){0.1, 0.1, 0.1, 1.0});
    if (server->background) {
        AXIOM_LOG_INFO("Background surface created");
    }
    
    // Initialize cursor manager safely
    if (server->cursor_mgr) {
        // Try to load with scale 1.0 first
        if (!wlr_xcursor_manager_load(server->cursor_mgr, 1.0)) {
            AXIOM_LOG_WARN("Failed to load cursor theme, will retry on first output");
        } else {
            // Set a default cursor immediately if possible
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
            AXIOM_LOG_INFO("Cursor theme loaded successfully");
        }
    }
}
```

### Fix 3: Proper Output Configuration

```c
// Update axiom_new_output() in compositor.c
void axiom_new_output(struct wl_listener *listener, void *data) {
    struct axiom_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;
    
    AXIOM_LOG_INFO("New output: %s", wlr_output->name);
    
    // Use the new wlroots 0.19 output state API
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    
    // Set preferred mode if available
    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        if (mode) {
            wlr_output_state_set_mode(&state, mode);
            wlr_output_state_set_enabled(&state, true);
            AXIOM_LOG_INFO("Output %s: %dx%d@%d", wlr_output->name,
                          mode->width, mode->height, mode->refresh);
        }
    } else {
        // Custom mode for outputs without predefined modes
        wlr_output_state_set_custom_mode(&state, 1920, 1080, 60000);
        wlr_output_state_set_enabled(&state, true);
        AXIOM_LOG_INFO("Output %s: using custom mode 1920x1080@60", wlr_output->name);
    }
    
    if (!wlr_output_commit_state(wlr_output, &state)) {
        AXIOM_LOG_ERROR("Failed to commit output %s", wlr_output->name);
        wlr_output_state_finish(&state);
        return;
    }
    
    wlr_output_state_finish(&state);
    
    // Create and configure the output
    struct wlr_output_layout_output *lo = wlr_output_layout_add_auto(server->output_layout, wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, lo, scene_output);
    
    // Update background size to cover the entire output layout
    struct wlr_box *layout_box = wlr_output_layout_get_box(server->output_layout, NULL);
    if (server->background && layout_box) {
        wlr_scene_rect_set_size(server->background, layout_box->width, layout_box->height);
        AXIOM_LOG_INFO("Background updated to %dx%d", layout_box->width, layout_box->height);
    }
    
    // Now that we have an output, try to load cursor theme if not already loaded
    if (server->cursor_mgr) {
        if (wlr_xcursor_manager_load(server->cursor_mgr, wlr_output->scale)) {
            wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "left_ptr");
            AXIOM_LOG_INFO("Cursor theme loaded for output %s", wlr_output->name);
        }
    }
}
```

### Fix 4: Session Script Improvements

```bash
# Add to axiom-session script before starting compositor
pre_flight_check() {
    log_info "Performing pre-flight checks..."
    
    # Check if we can create a Wayland socket
    if ! touch "${XDG_RUNTIME_DIR}/wayland-test" 2>/dev/null; then
        log_error "Cannot write to runtime directory: ${XDG_RUNTIME_DIR}"
        return 1
    fi
    rm -f "${XDG_RUNTIME_DIR}/wayland-test"
    
    # Check GPU access
    if ! ls /dev/dri/card* >/dev/null 2>&1; then
        log_error "No GPU cards found in /dev/dri/"
        return 1
    fi
    
    # Check if we can access input devices
    if ! ls /dev/input/event* >/dev/null 2>&1; then
        log_error "No input devices found in /dev/input/"
        return 1
    fi
    
    log_info "Pre-flight checks passed"
    return 0
}

# Add error handling wrapper
run_axiom_with_recovery() {
    local attempt=1
    local max_attempts=3
    
    while [ $attempt -le $max_attempts ]; do
        log_info "Starting Axiom (attempt $attempt/$max_attempts)"
        
        # Run Axiom with timeout to prevent hanging
        timeout 30s "${AXIOM_BINARY}" ${NESTED_MODE} 2>&1 | tee -a "${AXIOM_LOG_DIR}/session.log"
        local exit_code=$?
        
        case $exit_code in
            0)
                log_info "Axiom exited normally"
                return 0
                ;;
            124)
                log_error "Axiom startup timeout after 30 seconds"
                ;;
            *)
                log_error "Axiom crashed with exit code: $exit_code"
                ;;
        esac
        
        if [ $attempt -lt $max_attempts ]; then
            log_info "Waiting 5 seconds before retry..."
            sleep 5
        fi
        
        attempt=$((attempt + 1))
    done
    
    log_error "All attempts failed, giving up"
    return 1
}
```

## Immediate Action Plan

1. **Apply the event loop fix** - This is the most critical
2. **Add background surface creation** - Fixes the black screen
3. **Fix cursor initialization** - Prevents cursor-related crashes
4. **Test in nested mode first** - Verify fixes work
5. **Update session script** - Add error handling

## Quick Test Command

```bash
# Test the fixes
meson compile -C builddir && timeout 10s ./builddir/axiom --nested
```

This should show a gray background instead of black, and not crash immediately.

## Root Cause Summary

Your compositor crashes because:
1. **Event loop exits immediately on any error** (most critical)
2. **No background surface** creates confusing black screen
3. **Cursor initialization race conditions** cause segfaults
4. **Missing error recovery** in session handling

These fixes address the fundamental stability issues causing the SDDM kickback.
