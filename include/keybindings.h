#ifndef AXIOM_KEYBINDINGS_H
#define AXIOM_KEYBINDINGS_H

#include <stdint.h>
#include <stdbool.h>
#include <xkbcommon/xkbcommon.h>

struct axiom_server;

// Modifier key constants
#define AXIOM_MOD_SHIFT     (1 << 0)
#define AXIOM_MOD_CTRL      (1 << 1)
#define AXIOM_MOD_ALT       (1 << 2)
#define AXIOM_MOD_SUPER     (1 << 3)

// Maximum number of keybindings
#define AXIOM_MAX_KEYBINDINGS 128
#define AXIOM_MAX_COMMAND_LENGTH 256
#define AXIOM_MAX_MACRO_STEPS 16

// Action types
enum axiom_action_type {
    AXIOM_ACTION_COMMAND,           // Execute shell command
    AXIOM_ACTION_WINDOW_CLOSE,      // Close focused window
    AXIOM_ACTION_WINDOW_KILL,       // Force kill focused window
    AXIOM_ACTION_WINDOW_FULLSCREEN, // Toggle fullscreen
    AXIOM_ACTION_WINDOW_MAXIMIZE,   // Toggle maximize
    AXIOM_ACTION_WINDOW_FLOATING,   // Toggle floating
    AXIOM_ACTION_WINDOW_STICKY,     // Toggle sticky
    AXIOM_ACTION_LAYOUT_CYCLE,      // Cycle layouts
    AXIOM_ACTION_LAYOUT_SET,        // Set specific layout
    AXIOM_ACTION_MASTER_RATIO_INC,  // Increase master ratio
    AXIOM_ACTION_MASTER_RATIO_DEC,  // Decrease master ratio
    AXIOM_ACTION_TAG_VIEW,          // View specific tag
    AXIOM_ACTION_TAG_TOGGLE_VIEW,   // Toggle view tag
    AXIOM_ACTION_TAG_VIEW_ALL,      // View all tags
    AXIOM_ACTION_TAG_VIEW_PREVIOUS, // View previous tags
    AXIOM_ACTION_WINDOW_TAG,        // Tag focused window
    AXIOM_ACTION_WINDOW_TAG_TOGGLE, // Toggle tag on focused window
    AXIOM_ACTION_FOCUS_NEXT,        // Focus next window
    AXIOM_ACTION_FOCUS_PREV,        // Focus previous window
    AXIOM_ACTION_FOCUS_URGENT,      // Focus urgent window
    AXIOM_ACTION_QUIT,              // Quit compositor
    AXIOM_ACTION_RELOAD_CONFIG,     // Reload configuration
    AXIOM_ACTION_MACRO,             // Execute macro (sequence of actions)
};

// Macro step structure
struct axiom_macro_step {
    enum axiom_action_type action;
    int parameter;
    char command[AXIOM_MAX_COMMAND_LENGTH];
};

// Key binding structure
struct axiom_keybinding {
    uint32_t modifiers;             // Modifier keys (bitmask)
    xkb_keysym_t keysym;           // Key symbol
    enum axiom_action_type action;  // Action to perform
    int parameter;                  // Action parameter (tag number, layout, etc.)
    char command[AXIOM_MAX_COMMAND_LENGTH]; // Command for AXIOM_ACTION_COMMAND
    
    // Macro support
    bool is_macro;
    int macro_step_count;
    struct axiom_macro_step macro_steps[AXIOM_MAX_MACRO_STEPS];
    
    char description[128];          // Human-readable description
    bool enabled;                   // Can be disabled without removal
};

// Key binding manager
struct axiom_keybinding_manager {
    struct axiom_keybinding bindings[AXIOM_MAX_KEYBINDINGS];
    int binding_count;
    bool capture_mode;              // For learning new keybindings
    struct axiom_keybinding *pending_binding; // Binding being configured
};

// Key binding management functions
void axiom_keybinding_manager_init(struct axiom_keybinding_manager *manager);
void axiom_keybinding_manager_cleanup(struct axiom_keybinding_manager *manager);

// Binding registration
bool axiom_keybinding_add(struct axiom_keybinding_manager *manager,
                         uint32_t modifiers, xkb_keysym_t keysym,
                         enum axiom_action_type action, int parameter,
                         const char *command, const char *description);

bool axiom_keybinding_add_macro(struct axiom_keybinding_manager *manager,
                               uint32_t modifiers, xkb_keysym_t keysym,
                               const struct axiom_macro_step *steps, int step_count,
                               const char *description);

// Binding removal and modification
bool axiom_keybinding_remove(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym);
bool axiom_keybinding_enable(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym, bool enabled);
bool axiom_keybinding_update(struct axiom_keybinding_manager *manager,
                            uint32_t modifiers, xkb_keysym_t keysym,
                            enum axiom_action_type new_action, int new_parameter,
                            const char *new_command);

// Key processing
bool axiom_keybinding_handle_key(struct axiom_server *server,
                                uint32_t modifiers, xkb_keysym_t keysym);

// Action execution
void axiom_keybinding_execute_action(struct axiom_server *server,
                                   enum axiom_action_type action, int parameter,
                                   const char *command);
void axiom_keybinding_execute_macro(struct axiom_server *server,
                                  const struct axiom_macro_step *steps, int step_count);

// Configuration loading/saving
bool axiom_keybinding_load_config(struct axiom_keybinding_manager *manager, const char *config_file);
bool axiom_keybinding_save_config(struct axiom_keybinding_manager *manager, const char *config_file);

// Default bindings
void axiom_keybinding_load_defaults(struct axiom_keybinding_manager *manager);

// Utility functions
const char *axiom_keybinding_action_to_string(enum axiom_action_type action);
enum axiom_action_type axiom_keybinding_action_from_string(const char *action_str);
const char *axiom_keybinding_modifiers_to_string(uint32_t modifiers);
uint32_t axiom_keybinding_modifiers_from_string(const char *mod_str);
const char *axiom_keybinding_keysym_to_string(xkb_keysym_t keysym);
xkb_keysym_t axiom_keybinding_keysym_from_string(const char *key_str);

// Information and debugging
void axiom_keybinding_print_all(struct axiom_keybinding_manager *manager);
struct axiom_keybinding *axiom_keybinding_find(struct axiom_keybinding_manager *manager,
                                              uint32_t modifiers, xkb_keysym_t keysym);

#endif /* AXIOM_KEYBINDINGS_H */
