#ifndef AXIOM_INPUT_H
#define AXIOM_INPUT_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_touch.h>
#include <wlr/types/wlr_tablet_tool.h>
#include <wlr/types/wlr_tablet_pad.h>
#include <wlr/types/wlr_switch.h>
#include <wlr/types/wlr_text_input_v3.h>
#include <wlr/types/wlr_input_method_v2.h>
#include <xkbcommon/xkbcommon.h>

// Forward declarations
struct axiom_server;
struct axiom_window;

// Enhanced input device types
enum axiom_input_device_type {
    AXIOM_INPUT_KEYBOARD,
    AXIOM_INPUT_POINTER,
    AXIOM_INPUT_TOUCH,
    AXIOM_INPUT_TABLET_TOOL,
    AXIOM_INPUT_TABLET_PAD,
    AXIOM_INPUT_SWITCH,
    AXIOM_INPUT_UNKNOWN
};

// Input device capabilities
struct axiom_input_capabilities {
    bool has_keys;
    bool has_pointer;
    bool has_touch;
    bool has_tablet;
    bool has_gesture_support;
    bool has_scroll_wheel;
    bool has_tilt_detection;
    bool has_pressure_sensitivity;
    uint32_t max_touch_points;
};

// Enhanced keyboard state
struct axiom_keyboard_state {
    struct wlr_keyboard *wlr_keyboard;
    struct xkb_context *xkb_context;
    struct xkb_keymap *keymap;
    struct xkb_state *xkb_state;
    
    // Key repeat handling
    bool repeat_enabled;
    int32_t repeat_rate;
    int32_t repeat_delay;
    
    // Layout and locale
    char *layout;
    char *variant;
    char *options;
    
    // Accessibility features
    bool sticky_keys;
    bool slow_keys;
    bool bounce_keys;
    bool mouse_keys;
    
    // Event listeners
    struct wl_listener key;
    struct wl_listener modifiers;
    struct wl_listener repeat_info;
};

// Enhanced pointer state
struct axiom_pointer_state {
    struct wlr_pointer *wlr_pointer;
    
    // Acceleration and sensitivity
    double accel_speed;
    double sensitivity;
    bool natural_scroll;
    bool tap_to_click;
    bool click_method_button_areas;
    bool click_method_clickfinger;
    bool middle_emulation;
    bool disable_while_typing;
    
    // Gesture support
    bool gestures_enabled;
    double gesture_threshold;
    
    // Event listeners
    struct wl_listener motion;
    struct wl_listener motion_absolute;
    struct wl_listener button;
    struct wl_listener axis;
    struct wl_listener frame;
    struct wl_listener swipe_begin;
    struct wl_listener swipe_update;
    struct wl_listener swipe_end;
    struct wl_listener pinch_begin;
    struct wl_listener pinch_update;
    struct wl_listener pinch_end;
    struct wl_listener hold_begin;
    struct wl_listener hold_end;
};

// Touch point tracking
struct axiom_touch_point {
    uint32_t id;
    double x, y;
    bool active;
    uint32_t last_event_time;
};

// Touch input state
struct axiom_touch_state {
    struct wlr_touch *wlr_touch;
    
    // Multi-touch support
    struct axiom_touch_point points[10]; // Support up to 10 touch points
    uint32_t active_points;
    
    // Gesture recognition
    bool gestures_enabled;
    double gesture_threshold;
    uint32_t tap_timeout;
    uint32_t double_tap_timeout;
    
    // Event listeners
    struct wl_listener down;
    struct wl_listener up;
    struct wl_listener motion;
    struct wl_listener cancel;
    struct wl_listener frame;
};

// Tablet tool state
struct axiom_tablet_tool_state {
    struct wlr_tablet_tool *wlr_tool;
    
    // Tool properties
    bool has_pressure;
    bool has_distance;
    bool has_tilt;
    bool has_rotation;
    bool has_slider;
    bool has_wheel;
    
    // Calibration
    double pressure_curve[4]; // Bezier curve points
    double tilt_sensitivity;
    
    // Event listeners
    struct wl_listener axis;
    struct wl_listener proximity;
    struct wl_listener tip;
    struct wl_listener button;
};

// Tablet pad state
struct axiom_tablet_pad_state {
    struct wlr_tablet_pad *wlr_pad;
    
    // Button mappings
    uint32_t *button_map;
    size_t button_count;
    
    // Ring/strip mappings
    uint32_t *ring_map;
    size_t ring_count;
    uint32_t *strip_map;
    size_t strip_count;
    
    // Event listeners
    struct wl_listener button;
    struct wl_listener ring;
    struct wl_listener strip;
};

// Switch device state (lid, tablet mode, etc.)
struct axiom_switch_state {
    struct wlr_switch *wlr_switch;
    
    // Switch states
    bool lid_closed;
    bool tablet_mode;
    
    // Event listeners
    struct wl_listener toggle;
};

// Text input and IME support
struct axiom_input_method_state {
    struct wlr_text_input_v3 *text_input;
    struct wlr_input_method_v2 *input_method;
    
    // IME state
    bool ime_active;
    char *preedit_string;
    char *commit_string;
    
    // Language and locale
    char *language;
    char *region;
    
    // Event listeners
    struct wl_listener text_input_enable;
    struct wl_listener text_input_commit;
    struct wl_listener text_input_disable;
    struct wl_listener input_method_commit;
    struct wl_listener input_method_new_popup;
    struct wl_listener input_method_grab_keyboard;
};

// Accessibility features
struct axiom_accessibility_state {
    // Visual accessibility
    bool high_contrast;
    bool large_cursor;
    double cursor_size_multiplier;
    bool cursor_trails;
    
    // Audio accessibility
    bool visual_bell;
    bool sound_keys;
    
    // Motor accessibility
    bool slow_keys;
    bool sticky_keys;
    bool bounce_keys;
    bool mouse_keys;
    double slow_keys_delay;
    double sticky_keys_timeout;
    double bounce_keys_delay;
    
    // Focus assistance
    bool focus_follows_mouse;
    bool click_to_focus;
    bool sloppy_focus;
    double focus_delay;
};

// Enhanced input device structure (replaces the one in axiom.h)
struct axiom_enhanced_input_device {
    struct wl_list link;
    struct axiom_server *server;
    struct wlr_input_device *wlr_device;
    
    enum axiom_input_device_type type;
    struct axiom_input_capabilities caps;
    
    // Device-specific state
    union {
        struct axiom_keyboard_state keyboard;
        struct axiom_pointer_state pointer;
        struct axiom_touch_state touch;
        struct axiom_tablet_tool_state tablet_tool;
        struct axiom_tablet_pad_state tablet_pad;
        struct axiom_switch_state switch_device;
    };
    
    // Common properties
    char *name;
    char *identifier;
    bool enabled;
    
    // Device configuration
    struct wl_list config_link;
    
    // Event listeners
    struct wl_listener destroy;
};

// Input manager structure
struct axiom_input_manager {
    struct axiom_server *server;
    
    // Device lists
    struct wl_list keyboards;
    struct wl_list pointers;
    struct wl_list touch_devices;
    struct wl_list tablet_tools;
    struct wl_list tablet_pads;
    struct wl_list switches;
    
    // Current input state
    struct axiom_input_device *focused_keyboard;
    struct axiom_input_device *primary_pointer;
    
    // Text input and IME
    struct axiom_input_method_state ime;
    
    // Accessibility
    struct axiom_accessibility_state accessibility;
    
    // Configuration
    bool natural_scroll_default;
    bool tap_to_click_default;
    double pointer_accel_default;
    
    // Event listeners
    struct wl_listener new_input;
    struct wl_listener text_input_manager_enable;
    struct wl_listener input_method_manager_input_method;
};

// Function declarations

// Input manager lifecycle
struct axiom_input_manager *axiom_input_manager_create(struct axiom_server *server);
void axiom_input_manager_destroy(struct axiom_input_manager *manager);

// Device management
void axiom_input_manager_add_device(struct axiom_input_manager *manager, 
                                   struct wlr_input_device *device);
void axiom_input_manager_remove_device(struct axiom_input_manager *manager,
                                      struct axiom_input_device *device);
struct axiom_input_device *axiom_input_manager_get_device(struct axiom_input_manager *manager,
                                                         const char *identifier);

// Enhanced keyboard handling
void axiom_keyboard_create(struct axiom_input_manager *manager, 
                          struct wlr_input_device *device);
void axiom_keyboard_destroy(struct axiom_input_device *device);
void axiom_keyboard_set_layout(struct axiom_input_device *device, 
                              const char *layout, const char *variant);
void axiom_keyboard_set_repeat(struct axiom_input_device *device, 
                              int32_t rate, int32_t delay);
void axiom_keyboard_enable_accessibility(struct axiom_input_device *device,
                                        bool sticky_keys, bool slow_keys, bool bounce_keys);

// Enhanced pointer handling
void axiom_pointer_create(struct axiom_input_manager *manager,
                         struct wlr_input_device *device);
void axiom_pointer_destroy(struct axiom_input_device *device);
void axiom_pointer_set_accel(struct axiom_input_device *device, double speed);
void axiom_pointer_set_natural_scroll(struct axiom_input_device *device, bool enabled);
void axiom_pointer_set_tap_to_click(struct axiom_input_device *device, bool enabled);
void axiom_pointer_enable_gestures(struct axiom_input_device *device, bool enabled);

// Touch input handling
void axiom_touch_create(struct axiom_input_manager *manager,
                       struct wlr_input_device *device);
void axiom_touch_destroy(struct axiom_input_device *device);
void axiom_touch_enable_gestures(struct axiom_input_device *device, bool enabled);
struct axiom_touch_point *axiom_touch_get_point(struct axiom_input_device *device, uint32_t id);

// Tablet support
void axiom_tablet_tool_create(struct axiom_input_manager *manager,
                             struct wlr_input_device *device);
void axiom_tablet_tool_destroy(struct axiom_input_device *device);
void axiom_tablet_pad_create(struct axiom_input_manager *manager,
                            struct wlr_input_device *device);
void axiom_tablet_pad_destroy(struct axiom_input_device *device);
void axiom_tablet_tool_set_pressure_curve(struct axiom_input_device *device,
                                         double p1, double p2, double p3, double p4);

// Switch devices
void axiom_switch_create(struct axiom_input_manager *manager,
                        struct wlr_input_device *device);
void axiom_switch_destroy(struct axiom_input_device *device);

// Input method and IME support
void axiom_input_method_init(struct axiom_input_manager *manager);
void axiom_input_method_cleanup(struct axiom_input_manager *manager);
void axiom_input_method_set_language(struct axiom_input_manager *manager,
                                    const char *language, const char *region);
bool axiom_input_method_is_active(struct axiom_input_manager *manager);

// Accessibility support
void axiom_accessibility_init(struct axiom_input_manager *manager);
void axiom_accessibility_enable_high_contrast(struct axiom_input_manager *manager, bool enabled);
void axiom_accessibility_set_cursor_size(struct axiom_input_manager *manager, double multiplier);
void axiom_accessibility_enable_visual_bell(struct axiom_input_manager *manager, bool enabled);
void axiom_accessibility_enable_motor_assistance(struct axiom_input_manager *manager,
                                                bool sticky_keys, bool slow_keys, bool bounce_keys);

// Event handling
void axiom_input_handle_keyboard_key(struct wl_listener *listener, void *data);
void axiom_input_handle_keyboard_modifiers(struct wl_listener *listener, void *data);
void axiom_input_handle_pointer_motion(struct wl_listener *listener, void *data);
void axiom_input_handle_pointer_button(struct wl_listener *listener, void *data);
void axiom_input_handle_pointer_axis(struct wl_listener *listener, void *data);
void axiom_input_handle_touch_down(struct wl_listener *listener, void *data);
void axiom_input_handle_touch_up(struct wl_listener *listener, void *data);
void axiom_input_handle_touch_motion(struct wl_listener *listener, void *data);
void axiom_input_handle_tablet_tool_axis(struct wl_listener *listener, void *data);
void axiom_input_handle_tablet_tool_tip(struct wl_listener *listener, void *data);
void axiom_input_handle_switch_toggle(struct wl_listener *listener, void *data);

// Gesture recognition
enum axiom_gesture_type {
    AXIOM_GESTURE_NONE,
    AXIOM_GESTURE_TAP,
    AXIOM_GESTURE_DOUBLE_TAP,
    AXIOM_GESTURE_LONG_PRESS,
    AXIOM_GESTURE_SWIPE_LEFT,
    AXIOM_GESTURE_SWIPE_RIGHT,
    AXIOM_GESTURE_SWIPE_UP,
    AXIOM_GESTURE_SWIPE_DOWN,
    AXIOM_GESTURE_PINCH_IN,
    AXIOM_GESTURE_PINCH_OUT,
    AXIOM_GESTURE_ROTATE_CW,
    AXIOM_GESTURE_ROTATE_CCW
};

struct axiom_gesture_event {
    enum axiom_gesture_type type;
    double x, y;
    double dx, dy;
    double scale;
    double rotation;
    uint32_t finger_count;
    uint32_t time_msec;
};

typedef void (*axiom_gesture_handler_t)(struct axiom_server *server,
                                       struct axiom_gesture_event *event);

void axiom_input_register_gesture_handler(struct axiom_input_manager *manager,
                                         enum axiom_gesture_type type,
                                         axiom_gesture_handler_t handler);
void axiom_input_process_gesture(struct axiom_input_manager *manager,
                                struct axiom_gesture_event *event);

// Configuration
struct axiom_input_config {
    char *device_name;
    double accel_speed;
    bool natural_scroll;
    bool tap_to_click;
    bool gestures_enabled;
    char *keyboard_layout;
    char *keyboard_variant;
    bool accessibility_enabled;
    // Add more config options as needed
};

void axiom_input_apply_config(struct axiom_input_manager *manager,
                             struct axiom_input_config *config);
void axiom_input_save_config(struct axiom_input_manager *manager,
                            const char *config_path);
void axiom_input_load_config(struct axiom_input_manager *manager,
                            const char *config_path);

#endif /* AXIOM_INPUT_H */
