#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_pad.h>
#include <wlr/types/wlr_switch.h>
#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>
#include <math.h>
#include "axiom.h"
#include "input.h"
#include "keybindings.h"
#include "config.h"
#include "animation.h"
#include "pip_manager.h"
#include "logging.h"

// Forward declarations
static void input_device_destroy(struct wl_listener *listener, void *data);
static void detect_device_capabilities(struct axiom_input_device *device);

// =============================================================================
// INPUT MANAGER LIFECYCLE
// =============================================================================

struct axiom_input_manager *axiom_input_manager_create(struct axiom_server *server) {
    struct axiom_input_manager *manager = calloc(1, sizeof(struct axiom_input_manager));
    if (!manager) {
        AXIOM_LOG_ERROR("Failed to allocate input manager");
        return NULL;
    }

    manager->server = server;

    // Initialize device lists
    wl_list_init(&manager->keyboards);
    wl_list_init(&manager->pointers);
    wl_list_init(&manager->touch_devices);
    wl_list_init(&manager->tablet_tools);
    wl_list_init(&manager->tablet_pads);
    wl_list_init(&manager->switches);

    // Set default configurations
    manager->natural_scroll_default = false;
    manager->tap_to_click_default = true;
    manager->pointer_accel_default = 0.0;

    // Initialize accessibility features
    axiom_accessibility_init(manager);

    // Initialize input method support
    axiom_input_method_init(manager);

    AXIOM_LOG_INFO("Enhanced input manager created");
    return manager;
}

void axiom_input_manager_destroy(struct axiom_input_manager *manager) {
    if (!manager) {
        return;
    }

    // Clean up IME
    axiom_input_method_cleanup(manager);

    // Remove all devices
    struct axiom_input_device *device, *tmp;
    wl_list_for_each_safe(device, tmp, &manager->keyboards, link) {
        axiom_keyboard_destroy(device);
    }
    wl_list_for_each_safe(device, tmp, &manager->pointers, link) {
        axiom_pointer_destroy(device);
    }
    wl_list_for_each_safe(device, tmp, &manager->touch_devices, link) {
        axiom_touch_destroy(device);
    }
    wl_list_for_each_safe(device, tmp, &manager->tablet_tools, link) {
        axiom_tablet_tool_destroy(device);
    }
    wl_list_for_each_safe(device, tmp, &manager->tablet_pads, link) {
        axiom_tablet_pad_destroy(device);
    }
    wl_list_for_each_safe(device, tmp, &manager->switches, link) {
        axiom_switch_destroy(device);
    }

    free(manager);
    AXIOM_LOG_INFO("Input manager destroyed");
}

// =============================================================================
// DEVICE MANAGEMENT
// =============================================================================

void axiom_input_manager_add_device(struct axiom_input_manager *manager, 
                                   struct wlr_input_device *device) {
    if (!manager || !device) {
        return;
    }

    switch (device->type) {
    case WLR_INPUT_DEVICE_KEYBOARD:
        axiom_keyboard_create(manager, device);
        break;
    case WLR_INPUT_DEVICE_POINTER:
        axiom_pointer_create(manager, device);
        break;
    case WLR_INPUT_DEVICE_TOUCH:
        axiom_touch_create(manager, device);
        break;
    case WLR_INPUT_DEVICE_TABLET_TOOL:
        axiom_tablet_tool_create(manager, device);
        break;
    case WLR_INPUT_DEVICE_TABLET_PAD:
        axiom_tablet_pad_create(manager, device);
        break;
    case WLR_INPUT_DEVICE_SWITCH:
        axiom_switch_create(manager, device);
        break;
    default:
        AXIOM_LOG_INFO("Unsupported input device type: %d", device->type);
        break;
    }
}

void axiom_input_manager_remove_device(struct axiom_input_manager *manager,
                                      struct axiom_input_device *device) {
    if (!manager || !device) {
        return;
    }

    switch (device->type) {
    case AXIOM_INPUT_KEYBOARD:
        axiom_keyboard_destroy(device);
        break;
    case AXIOM_INPUT_POINTER:
        axiom_pointer_destroy(device);
        break;
    case AXIOM_INPUT_TOUCH:
        axiom_touch_destroy(device);
        break;
    case AXIOM_INPUT_TABLET_TOOL:
        axiom_tablet_tool_destroy(device);
        break;
    case AXIOM_INPUT_TABLET_PAD:
        axiom_tablet_pad_destroy(device);
        break;
    case AXIOM_INPUT_SWITCH:
        axiom_switch_destroy(device);
        break;
    default:
        break;
    }
}

struct axiom_input_device *axiom_input_manager_get_device(struct axiom_input_manager *manager,
                                                         const char *identifier) {
    if (!manager || !identifier) {
        return NULL;
    }

    struct axiom_input_device *device;
    
    // Search keyboards
    wl_list_for_each(device, &manager->keyboards, link) {
        if (device->identifier && strcmp(device->identifier, identifier) == 0) {
            return device;
        }
    }
    
    // Search pointers
    wl_list_for_each(device, &manager->pointers, link) {
        if (device->identifier && strcmp(device->identifier, identifier) == 0) {
            return device;
        }
    }
    
    // Search other device types...
    
    return NULL;
}

// =============================================================================
// KEYBOARD HANDLING
// =============================================================================

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, keyboard.key);
    struct axiom_server *server = device->server;
    struct wlr_keyboard_key_event *event = data;
    struct wlr_seat *seat = server->seat;
    struct wlr_keyboard *keyboard = device->keyboard.wlr_keyboard;
    
    uint32_t keycode = event->keycode + 8;
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(keyboard->xkb_state, keycode, &syms);
    
    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard);
    
    // Handle accessibility features
    if (device->keyboard.sticky_keys || device->keyboard.slow_keys || device->keyboard.bounce_keys) {
        // Implement accessibility key handling here
        // This would include timing checks for slow keys, modifier persistence for sticky keys, etc.
    }
    
    if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        for (int i = 0; i < nsyms; i++) {
            // Try new keybinding system first
            if (server->keybinding_manager) {
                handled = axiom_keybinding_handle_key(server, modifiers, syms[i]);
                if (handled) break;
            }
        }
    }
    
    if (!handled) {
        wlr_seat_set_keyboard(seat, keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
    }
}

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, keyboard.modifiers);
    struct wlr_keyboard *keyboard = device->keyboard.wlr_keyboard;
    wlr_seat_set_keyboard(device->server->seat, keyboard);
    wlr_seat_keyboard_notify_modifiers(device->server->seat, &keyboard->modifiers);
}

void axiom_keyboard_create(struct axiom_input_manager *manager, 
                          struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate keyboard device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_KEYBOARD;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name); // Simple identifier for now

    struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(wlr_device);
    device->keyboard.wlr_keyboard = wlr_keyboard;

    // Create XKB context and keymap
    device->keyboard.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!device->keyboard.xkb_context) {
        AXIOM_LOG_ERROR("Failed to create XKB context");
        free(device->name);
        free(device->identifier);
        free(device);
        return;
    }

    struct xkb_keymap *keymap = xkb_keymap_new_from_names(device->keyboard.xkb_context, 
                                                         NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        AXIOM_LOG_ERROR("Failed to create XKB keymap");
        xkb_context_unref(device->keyboard.xkb_context);
        free(device->name);
        free(device->identifier);
        free(device);
        return;
    }

    device->keyboard.keymap = keymap;
    wlr_keyboard_set_keymap(wlr_keyboard, keymap);

    // Set repeat info from config
    device->keyboard.repeat_enabled = true;
    device->keyboard.repeat_rate = manager->server->config->repeat_rate;
    device->keyboard.repeat_delay = manager->server->config->repeat_delay;
    wlr_keyboard_set_repeat_info(wlr_keyboard, device->keyboard.repeat_rate, device->keyboard.repeat_delay);

    // Setup event listeners
    device->keyboard.key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &device->keyboard.key);
    device->keyboard.modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &device->keyboard.modifiers);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->keyboards, &device->link);
    wlr_seat_set_keyboard(manager->server->seat, wlr_keyboard);

    // Set as focused keyboard if none exists
    if (!manager->focused_keyboard) {
        manager->focused_keyboard = device;
    }

    AXIOM_LOG_INFO("Enhanced keyboard created: %s", device->name);
}

void axiom_keyboard_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_KEYBOARD) {
        return;
    }

    // Remove from manager's focused keyboard if it's this device
    if (device->server->input_manager->focused_keyboard == device) {
        device->server->input_manager->focused_keyboard = NULL;
    }

    // Cleanup XKB resources
    if (device->keyboard.keymap) {
        xkb_keymap_unref(device->keyboard.keymap);
    }
    if (device->keyboard.xkb_context) {
        xkb_context_unref(device->keyboard.xkb_context);
    }

    // Free strings
    free(device->keyboard.layout);
    free(device->keyboard.variant);
    free(device->keyboard.options);

    // Remove listeners
    wl_list_remove(&device->keyboard.key.link);
    wl_list_remove(&device->keyboard.modifiers.link);
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

void axiom_keyboard_set_layout(struct axiom_input_device *device, 
                              const char *layout, const char *variant) {
    if (!device || device->type != AXIOM_INPUT_KEYBOARD) {
        return;
    }

    free(device->keyboard.layout);
    free(device->keyboard.variant);
    device->keyboard.layout = layout ? strdup(layout) : NULL;
    device->keyboard.variant = variant ? strdup(variant) : NULL;

    // Recreate keymap with new layout
    struct xkb_rule_names rules = {
        .layout = device->keyboard.layout,
        .variant = device->keyboard.variant,
        .options = device->keyboard.options,
    };

    struct xkb_keymap *new_keymap = xkb_keymap_new_from_names(device->keyboard.xkb_context,
                                                             &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (new_keymap) {
        xkb_keymap_unref(device->keyboard.keymap);
        device->keyboard.keymap = new_keymap;
        wlr_keyboard_set_keymap(device->keyboard.wlr_keyboard, new_keymap);
        AXIOM_LOG_INFO("Keyboard layout updated: %s %s", layout ?: "default", variant ?: "");
    } else {
        AXIOM_LOG_ERROR("Failed to create keymap for layout: %s %s", layout ?: "default", variant ?: "");
    }
}

void axiom_keyboard_set_repeat(struct axiom_input_device *device, 
                              int32_t rate, int32_t delay) {
    if (!device || device->type != AXIOM_INPUT_KEYBOARD) {
        return;
    }

    device->keyboard.repeat_rate = rate;
    device->keyboard.repeat_delay = delay;
    wlr_keyboard_set_repeat_info(device->keyboard.wlr_keyboard, rate, delay);
}

void axiom_keyboard_enable_accessibility(struct axiom_input_device *device,
                                        bool sticky_keys, bool slow_keys, bool bounce_keys) {
    if (!device || device->type != AXIOM_INPUT_KEYBOARD) {
        return;
    }

    device->keyboard.sticky_keys = sticky_keys;
    device->keyboard.slow_keys = slow_keys;
    device->keyboard.bounce_keys = bounce_keys;

    AXIOM_LOG_INFO("Keyboard accessibility enabled - sticky: %d, slow: %d, bounce: %d",
                   sticky_keys, slow_keys, bounce_keys);
}

// =============================================================================
// POINTER HANDLING
// =============================================================================

static void pointer_handle_motion(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, pointer.motion);
    struct wlr_pointer_motion_event *event = data;
    
    // Apply acceleration and sensitivity
    double dx = event->delta_x * device->pointer.sensitivity;
    double dy = event->delta_y * device->pointer.sensitivity;
    
    wlr_cursor_move(device->server->cursor, &event->pointer->base, dx, dy);
    // Call existing cursor motion processing
    axiom_cursor_motion(NULL, data);
}

static void pointer_handle_button(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, pointer.button);
    // Forward to existing button handler
    axiom_cursor_button(NULL, data);
}

static void pointer_handle_axis(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, pointer.axis);
    struct wlr_pointer_axis_event *event = data;
    
    // Apply natural scroll if enabled
    if (device->pointer.natural_scroll) {
        event->delta = -event->delta;
        event->delta_discrete = -event->delta_discrete;
    }
    
    // Forward to existing axis handler
    axiom_cursor_axis(NULL, data);
}

void axiom_pointer_create(struct axiom_input_manager *manager,
                         struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate pointer device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_POINTER;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name);

    device->pointer.wlr_pointer = wlr_pointer_from_input_device(wlr_device);
    
    // Set default pointer settings
    device->pointer.accel_speed = manager->pointer_accel_default;
    device->pointer.sensitivity = 1.0;
    device->pointer.natural_scroll = manager->natural_scroll_default;
    device->pointer.tap_to_click = manager->tap_to_click_default;
    device->pointer.gestures_enabled = true;
    device->pointer.gesture_threshold = 10.0;

    // Setup event listeners
    device->pointer.motion.notify = pointer_handle_motion;
    wl_signal_add(&device->pointer.wlr_pointer->events.motion, &device->pointer.motion);
    
    device->pointer.button.notify = pointer_handle_button;
    wl_signal_add(&device->pointer.wlr_pointer->events.button, &device->pointer.button);
    
    device->pointer.axis.notify = pointer_handle_axis;
    wl_signal_add(&device->pointer.wlr_pointer->events.axis, &device->pointer.axis);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->pointers, &device->link);
    wlr_cursor_attach_input_device(manager->server->cursor, wlr_device);

    // Set as primary pointer if none exists
    if (!manager->primary_pointer) {
        manager->primary_pointer = device;
    }

    AXIOM_LOG_INFO("Enhanced pointer created: %s", device->name);
}

void axiom_pointer_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_POINTER) {
        return;
    }

    // Remove from manager's primary pointer if it's this device
    if (device->server->input_manager->primary_pointer == device) {
        device->server->input_manager->primary_pointer = NULL;
    }

    // Remove listeners
    wl_list_remove(&device->pointer.motion.link);
    wl_list_remove(&device->pointer.button.link);
    wl_list_remove(&device->pointer.axis.link);
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

void axiom_pointer_set_accel(struct axiom_input_device *device, double speed) {
    if (!device || device->type != AXIOM_INPUT_POINTER) {
        return;
    }

    device->pointer.accel_speed = speed;
    // Apply to wlroots pointer if supported
    if (wlr_input_device_is_libinput(device->wlr_device)) {
        // Set libinput acceleration
    }
}

void axiom_pointer_set_natural_scroll(struct axiom_input_device *device, bool enabled) {
    if (!device || device->type != AXIOM_INPUT_POINTER) {
        return;
    }

    device->pointer.natural_scroll = enabled;
}

void axiom_pointer_set_tap_to_click(struct axiom_input_device *device, bool enabled) {
    if (!device || device->type != AXIOM_INPUT_POINTER) {
        return;
    }

    device->pointer.tap_to_click = enabled;
}

void axiom_pointer_enable_gestures(struct axiom_input_device *device, bool enabled) {
    if (!device || device->type != AXIOM_INPUT_POINTER) {
        return;
    }

    device->pointer.gestures_enabled = enabled;
}

// =============================================================================
// TOUCH HANDLING
// =============================================================================

static void touch_handle_down(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, touch.down);
    struct wlr_touch_down_event *event = data;
    
    // Find available touch point slot
    for (int i = 0; i < 10; i++) {
        if (!device->touch.points[i].active) {
            device->touch.points[i].id = event->touch_id;
            device->touch.points[i].x = event->x;
            device->touch.points[i].y = event->y;
            device->touch.points[i].active = true;
            device->touch.points[i].last_event_time = event->time_msec;
            device->touch.active_points++;
            break;
        }
    }
    
    wlr_seat_touch_notify_down(device->server->seat, event->surface,
                               event->time_msec, event->touch_id, event->x, event->y);
}

static void touch_handle_up(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, touch.up);
    struct wlr_touch_up_event *event = data;
    
    // Find and deactivate touch point
    for (int i = 0; i < 10; i++) {
        if (device->touch.points[i].active && device->touch.points[i].id == event->touch_id) {
            device->touch.points[i].active = false;
            device->touch.active_points--;
            break;
        }
    }
    
    wlr_seat_touch_notify_up(device->server->seat, event->time_msec, event->touch_id);
}

static void touch_handle_motion(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, touch.motion);
    struct wlr_touch_motion_event *event = data;
    
    // Update touch point position
    for (int i = 0; i < 10; i++) {
        if (device->touch.points[i].active && device->touch.points[i].id == event->touch_id) {
            device->touch.points[i].x = event->x;
            device->touch.points[i].y = event->y;
            device->touch.points[i].last_event_time = event->time_msec;
            break;
        }
    }
    
    wlr_seat_touch_notify_motion(device->server->seat, event->time_msec,
                                 event->touch_id, event->x, event->y);
}

void axiom_touch_create(struct axiom_input_manager *manager,
                       struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate touch device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_TOUCH;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name);

    device->touch.wlr_touch = wlr_touch_from_input_device(wlr_device);
    device->touch.gestures_enabled = true;
    device->touch.gesture_threshold = 20.0;
    device->touch.tap_timeout = 200;
    device->touch.double_tap_timeout = 400;

    // Initialize touch points
    for (int i = 0; i < 10; i++) {
        device->touch.points[i].active = false;
    }

    // Setup event listeners
    device->touch.down.notify = touch_handle_down;
    wl_signal_add(&device->touch.wlr_touch->events.down, &device->touch.down);
    
    device->touch.up.notify = touch_handle_up;
    wl_signal_add(&device->touch.wlr_touch->events.up, &device->touch.up);
    
    device->touch.motion.notify = touch_handle_motion;
    wl_signal_add(&device->touch.wlr_touch->events.motion, &device->touch.motion);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->touch_devices, &device->link);

    AXIOM_LOG_INFO("Touch device created: %s", device->name);
}

void axiom_touch_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_TOUCH) {
        return;
    }

    // Remove listeners
    wl_list_remove(&device->touch.down.link);
    wl_list_remove(&device->touch.up.link);
    wl_list_remove(&device->touch.motion.link);
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

void axiom_touch_enable_gestures(struct axiom_input_device *device, bool enabled) {
    if (!device || device->type != AXIOM_INPUT_TOUCH) {
        return;
    }

    device->touch.gestures_enabled = enabled;
}

struct axiom_touch_point *axiom_touch_get_point(struct axiom_input_device *device, uint32_t id) {
    if (!device || device->type != AXIOM_INPUT_TOUCH) {
        return NULL;
    }

    for (int i = 0; i < 10; i++) {
        if (device->touch.points[i].active && device->touch.points[i].id == id) {
            return &device->touch.points[i];
        }
    }
    return NULL;
}

// =============================================================================
// TABLET SUPPORT
// =============================================================================

static void tablet_tool_handle_axis(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, tablet_tool.axis);
    struct wlr_tablet_tool_axis_event *event = data;
    
    // Apply pressure curve if pressure changed
    if (event->updated_axes & WLR_TABLET_TOOL_AXIS_PRESSURE) {
        // Apply Bezier curve transformation to pressure
        double t = event->pressure;
        double p0 = device->tablet_tool.pressure_curve[0];
        double p1 = device->tablet_tool.pressure_curve[1];
        double p2 = device->tablet_tool.pressure_curve[2];
        double p3 = device->tablet_tool.pressure_curve[3];
        
        // Cubic Bezier curve formula
        double transformed_pressure = pow(1-t, 3) * p0 + 3*pow(1-t, 2)*t * p1 + 
                                     3*(1-t)*pow(t, 2) * p2 + pow(t, 3) * p3;
        
        // Update event with transformed pressure
        ((struct wlr_tablet_tool_axis_event*)event)->pressure = transformed_pressure;
    }
    
    wlr_cursor_warp_absolute(device->server->cursor, &event->tablet->base, event->x, event->y);
    wlr_seat_tablet_notify_axis(device->server->seat, event->tool, event->time_msec,
                                event->updated_axes, event->x, event->y, event->dx, event->dy,
                                event->pressure, event->distance, event->tilt_x, event->tilt_y,
                                event->rotation, event->slider, event->wheel_delta);
}

static void tablet_tool_handle_tip(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, tablet_tool.tip);
    struct wlr_tablet_tool_tip_event *event = data;
    
    wlr_seat_tablet_notify_tip(device->server->seat, event->tool, event->time_msec, event->state);
}

void axiom_tablet_tool_create(struct axiom_input_manager *manager,
                             struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate tablet tool device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_TABLET_TOOL;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name);

    device->tablet_tool.wlr_tool = wlr_tablet_from_input_device(wlr_device);
    
    // Set default pressure curve (linear)
    device->tablet_tool.pressure_curve[0] = 0.0;
    device->tablet_tool.pressure_curve[1] = 0.33;
    device->tablet_tool.pressure_curve[2] = 0.66;
    device->tablet_tool.pressure_curve[3] = 1.0;
    
    device->tablet_tool.tilt_sensitivity = 1.0;

    // Setup event listeners
    device->tablet_tool.axis.notify = tablet_tool_handle_axis;
    wl_signal_add(&device->tablet_tool.wlr_tool->events.axis, &device->tablet_tool.axis);
    
    device->tablet_tool.tip.notify = tablet_tool_handle_tip;
    wl_signal_add(&device->tablet_tool.wlr_tool->events.tip, &device->tablet_tool.tip);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->tablet_tools, &device->link);

    AXIOM_LOG_INFO("Tablet tool created: %s", device->name);
}

void axiom_tablet_tool_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_TABLET_TOOL) {
        return;
    }

    // Remove listeners
    wl_list_remove(&device->tablet_tool.axis.link);
    wl_list_remove(&device->tablet_tool.tip.link);
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

void axiom_tablet_pad_create(struct axiom_input_manager *manager,
                            struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate tablet pad device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_TABLET_PAD;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name);

    device->tablet_pad.wlr_pad = wlr_tablet_pad_from_input_device(wlr_device);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->tablet_pads, &device->link);

    AXIOM_LOG_INFO("Tablet pad created: %s", device->name);
}

void axiom_tablet_pad_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_TABLET_PAD) {
        return;
    }

    // Free button mappings
    free(device->tablet_pad.button_map);
    free(device->tablet_pad.ring_map);
    free(device->tablet_pad.strip_map);

    // Remove listeners
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

void axiom_tablet_tool_set_pressure_curve(struct axiom_input_device *device,
                                         double p1, double p2, double p3, double p4) {
    if (!device || device->type != AXIOM_INPUT_TABLET_TOOL) {
        return;
    }

    device->tablet_tool.pressure_curve[0] = p1;
    device->tablet_tool.pressure_curve[1] = p2;
    device->tablet_tool.pressure_curve[2] = p3;
    device->tablet_tool.pressure_curve[3] = p4;
}

// =============================================================================
// SWITCH DEVICES
// =============================================================================

static void switch_handle_toggle(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, switch_device.toggle);
    struct wlr_switch_toggle_event *event = data;
    
    switch (event->switch_type) {
    case WLR_SWITCH_TYPE_LID:
        device->switch_device.lid_closed = (event->switch_state == WLR_SWITCH_STATE_ON);
        AXIOM_LOG_INFO("Lid switch: %s", device->switch_device.lid_closed ? "closed" : "open");
        break;
    case WLR_SWITCH_TYPE_TABLET_MODE:
        device->switch_device.tablet_mode = (event->switch_state == WLR_SWITCH_STATE_ON);
        AXIOM_LOG_INFO("Tablet mode: %s", device->switch_device.tablet_mode ? "enabled" : "disabled");
        break;
    }
}

void axiom_switch_create(struct axiom_input_manager *manager,
                        struct wlr_input_device *wlr_device) {
    struct axiom_input_device *device = calloc(1, sizeof(struct axiom_input_device));
    if (!device) {
        AXIOM_LOG_ERROR("Failed to allocate switch device");
        return;
    }

    device->server = manager->server;
    device->wlr_device = wlr_device;
    device->type = AXIOM_INPUT_SWITCH;
    device->enabled = true;
    device->name = strdup(wlr_device->name);
    device->identifier = strdup(wlr_device->name);

    device->switch_device.wlr_switch = wlr_switch_from_input_device(wlr_device);

    // Setup event listeners
    device->switch_device.toggle.notify = switch_handle_toggle;
    wl_signal_add(&device->switch_device.wlr_switch->events.toggle, &device->switch_device.toggle);

    device->destroy.notify = input_device_destroy;
    wl_signal_add(&wlr_device->events.destroy, &device->destroy);

    detect_device_capabilities(device);
    wl_list_insert(&manager->switches, &device->link);

    AXIOM_LOG_INFO("Switch device created: %s", device->name);
}

void axiom_switch_destroy(struct axiom_input_device *device) {
    if (!device || device->type != AXIOM_INPUT_SWITCH) {
        return;
    }

    // Remove listeners
    wl_list_remove(&device->switch_device.toggle.link);
    wl_list_remove(&device->destroy.link);
    wl_list_remove(&device->link);

    free(device->name);
    free(device->identifier);
    free(device);
}

// =============================================================================
// INPUT METHOD AND IME SUPPORT
// =============================================================================

void axiom_input_method_init(struct axiom_input_manager *manager) {
    if (!manager) {
        return;
    }

    // Initialize IME state
    manager->ime.ime_active = false;
    manager->ime.preedit_string = NULL;
    manager->ime.commit_string = NULL;
    manager->ime.language = strdup("en");
    manager->ime.region = strdup("US");

    AXIOM_LOG_INFO("Input method framework initialized");
}

void axiom_input_method_cleanup(struct axiom_input_manager *manager) {
    if (!manager) {
        return;
    }

    free(manager->ime.preedit_string);
    free(manager->ime.commit_string);
    free(manager->ime.language);
    free(manager->ime.region);
}

void axiom_input_method_set_language(struct axiom_input_manager *manager,
                                    const char *language, const char *region) {
    if (!manager) {
        return;
    }

    free(manager->ime.language);
    free(manager->ime.region);
    manager->ime.language = language ? strdup(language) : NULL;
    manager->ime.region = region ? strdup(region) : NULL;

    AXIOM_LOG_INFO("Input method language set: %s_%s", language ?: "default", region ?: "default");
}

bool axiom_input_method_is_active(struct axiom_input_manager *manager) {
    return manager ? manager->ime.ime_active : false;
}

// =============================================================================
// ACCESSIBILITY SUPPORT
// =============================================================================

void axiom_accessibility_init(struct axiom_input_manager *manager) {
    if (!manager) {
        return;
    }

    // Initialize accessibility state with defaults
    manager->accessibility.high_contrast = false;
    manager->accessibility.large_cursor = false;
    manager->accessibility.cursor_size_multiplier = 1.0;
    manager->accessibility.cursor_trails = false;
    manager->accessibility.visual_bell = false;
    manager->accessibility.sound_keys = false;
    manager->accessibility.slow_keys = false;
    manager->accessibility.sticky_keys = false;
    manager->accessibility.bounce_keys = false;
    manager->accessibility.mouse_keys = false;
    manager->accessibility.slow_keys_delay = 0.5;
    manager->accessibility.sticky_keys_timeout = 5.0;
    manager->accessibility.bounce_keys_delay = 0.1;
    manager->accessibility.focus_follows_mouse = false;
    manager->accessibility.click_to_focus = true;
    manager->accessibility.sloppy_focus = false;
    manager->accessibility.focus_delay = 0.0;

    AXIOM_LOG_INFO("Accessibility features initialized");
}

void axiom_accessibility_enable_high_contrast(struct axiom_input_manager *manager, bool enabled) {
    if (!manager) {
        return;
    }

    manager->accessibility.high_contrast = enabled;
    AXIOM_LOG_INFO("High contrast %s", enabled ? "enabled" : "disabled");
}

void axiom_accessibility_set_cursor_size(struct axiom_input_manager *manager, double multiplier) {
    if (!manager) {
        return;
    }

    manager->accessibility.cursor_size_multiplier = multiplier;
    manager->accessibility.large_cursor = (multiplier > 1.0);
    AXIOM_LOG_INFO("Cursor size multiplier set to %.2f", multiplier);
}

void axiom_accessibility_enable_visual_bell(struct axiom_input_manager *manager, bool enabled) {
    if (!manager) {
        return;
    }

    manager->accessibility.visual_bell = enabled;
    AXIOM_LOG_INFO("Visual bell %s", enabled ? "enabled" : "disabled");
}

void axiom_accessibility_enable_motor_assistance(struct axiom_input_manager *manager,
                                                bool sticky_keys, bool slow_keys, bool bounce_keys) {
    if (!manager) {
        return;
    }

    manager->accessibility.sticky_keys = sticky_keys;
    manager->accessibility.slow_keys = slow_keys;
    manager->accessibility.bounce_keys = bounce_keys;

    AXIOM_LOG_INFO("Motor assistance - sticky: %d, slow: %d, bounce: %d",
                   sticky_keys, slow_keys, bounce_keys);
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

static void input_device_destroy(struct wl_listener *listener, void *data) {
    struct axiom_input_device *device = wl_container_of(listener, device, destroy);
    axiom_input_manager_remove_device(device->server->input_manager, device);
}

static void detect_device_capabilities(struct axiom_input_device *device) {
    if (!device) {
        return;
    }

    // Reset capabilities
    memset(&device->caps, 0, sizeof(device->caps));

    switch (device->type) {
    case AXIOM_INPUT_KEYBOARD:
        device->caps.has_keys = true;
        break;
    case AXIOM_INPUT_POINTER:
        device->caps.has_pointer = true;
        device->caps.has_scroll_wheel = true; // Assume most pointers have scroll
        break;
    case AXIOM_INPUT_TOUCH:
        device->caps.has_touch = true;
        device->caps.has_gesture_support = true;
        device->caps.max_touch_points = 10;
        break;
    case AXIOM_INPUT_TABLET_TOOL:
        device->caps.has_tablet = true;
        device->caps.has_pressure_sensitivity = true;
        device->caps.has_tilt_detection = true;
        break;
    case AXIOM_INPUT_TABLET_PAD:
        device->caps.has_tablet = true;
        break;
    case AXIOM_INPUT_SWITCH:
        // No specific capabilities for switches
        break;
    default:
        break;
    }
}
