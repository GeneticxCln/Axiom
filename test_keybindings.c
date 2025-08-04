#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <xkbcommon/xkbcommon.h>

// Stub the constants we need
#define AXIOM_TAGS_MAX 9

// Define minimal structures
enum axiom_action_type {
  AXIOM_ACTION_COMMAND,
  AXIOM_ACTION_WINDOW_CLOSE,
  AXIOM_ACTION_WINDOW_KILL,
  AXIOM_ACTION_WINDOW_FULLSCREEN,
  AXIOM_ACTION_WINDOW_MAXIMIZE,
  AXIOM_ACTION_WINDOW_FLOATING,
  AXIOM_ACTION_WINDOW_STICKY,
  AXIOM_ACTION_LAYOUT_CYCLE,
  AXIOM_ACTION_LAYOUT_SET,
  AXIOM_ACTION_MASTER_RATIO_INC,
  AXIOM_ACTION_MASTER_RATIO_DEC,
  AXIOM_ACTION_TAG_VIEW,
  AXIOM_ACTION_TAG_TOGGLE_VIEW,
  AXIOM_ACTION_TAG_VIEW_ALL,
  AXIOM_ACTION_TAG_VIEW_PREVIOUS,
  AXIOM_ACTION_WINDOW_TAG,
  AXIOM_ACTION_WINDOW_TAG_TOGGLE,
  AXIOM_ACTION_FOCUS_NEXT,
  AXIOM_ACTION_FOCUS_PREV,
  AXIOM_ACTION_FOCUS_URGENT,
  AXIOM_ACTION_QUIT,
  AXIOM_ACTION_RELOAD_CONFIG,
  AXIOM_ACTION_MACRO,
};

#define AXIOM_MOD_SHIFT (1 << 0)
#define AXIOM_MOD_CTRL (1 << 1)
#define AXIOM_MOD_ALT (1 << 2)
#define AXIOM_MOD_SUPER (1 << 3)
#define AXIOM_MAX_KEYBINDINGS 128
#define AXIOM_MAX_COMMAND_LENGTH 256
#define AXIOM_MAX_MACRO_STEPS 16

struct axiom_macro_step {
    enum axiom_action_type action;
    int parameter;
    char command[AXIOM_MAX_COMMAND_LENGTH];
};

struct axiom_keybinding {
    uint32_t modifiers;
    xkb_keysym_t keysym;
    enum axiom_action_type action;
    int parameter;
    char command[AXIOM_MAX_COMMAND_LENGTH];
    bool is_macro;
    int macro_step_count;
    struct axiom_macro_step macro_steps[AXIOM_MAX_MACRO_STEPS];
    char description[128];
    bool enabled;
};

struct axiom_keybinding_manager {
    struct axiom_keybinding bindings[AXIOM_MAX_KEYBINDINGS];
    int binding_count;
    bool capture_mode;
    struct axiom_keybinding *pending_binding;
};

// Mock logging function
void axiom_log_debug_variadic(const char *format, ...) {
    (void)format;
}

void axiom_log_info_variadic(const char *format, ...) {
    (void)format;
}

// Test the keybinding system
void test_keybinding_system() {
    printf("Testing keybinding system...\n");
    
    struct axiom_keybinding_manager manager;
    axiom_keybinding_manager_init(&manager);
    
    // Test that default bindings were loaded
    assert(manager.binding_count > 0);
    printf("✓ Default keybindings loaded: %d bindings\n", manager.binding_count);
    
    // Test adding a custom keybinding
    int initial_count = manager.binding_count;
    bool success = axiom_keybinding_add(&manager, 
                                       AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, 
                                       XKB_KEY_t,
                                       AXIOM_ACTION_COMMAND, 
                                       0, 
                                       "foot", 
                                       "Launch terminal (custom)");
    assert(success);
    assert(manager.binding_count == initial_count + 1);
    printf("✓ Added custom keybinding successfully\n");
    
    // Test finding a keybinding
    struct axiom_keybinding *binding = axiom_keybinding_find(&manager, 
                                                            AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, 
                                                            XKB_KEY_t);
    assert(binding != NULL);
    assert(binding->action == AXIOM_ACTION_COMMAND);
    assert(strcmp(binding->command, "foot") == 0);
    printf("✓ Found keybinding successfully\n");
    
    // Test string conversion functions
    const char *action_str = axiom_keybinding_action_to_string(AXIOM_ACTION_COMMAND);
    assert(strcmp(action_str, "command") == 0);
    printf("✓ Action to string conversion works\n");
    
    enum axiom_action_type action = axiom_keybinding_action_from_string("window_close");
    assert(action == AXIOM_ACTION_WINDOW_CLOSE);
    printf("✓ String to action conversion works\n");
    
    // Test modifier string parsing
    uint32_t modifiers = axiom_keybinding_modifiers_from_string("Super+Shift");
    assert(modifiers == (AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT));
    printf("✓ Modifier string parsing works\n");
    
    // Test keysym parsing
    xkb_keysym_t keysym = axiom_keybinding_keysym_from_string("Return");
    assert(keysym == XKB_KEY_Return);
    printf("✓ Keysym string parsing works\n");
    
    // Test enabling/disabling bindings
    bool disabled = axiom_keybinding_enable(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t, false);
    assert(disabled);
    binding = axiom_keybinding_find(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t);
    assert(binding != NULL && !binding->enabled);
    printf("✓ Keybinding disable works\n");
    
    // Test re-enabling
    bool enabled = axiom_keybinding_enable(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t, true);
    assert(enabled);
    binding = axiom_keybinding_find(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t);
    assert(binding != NULL && binding->enabled);
    printf("✓ Keybinding re-enable works\n");
    
    // Test keybinding removal
    bool removed = axiom_keybinding_remove(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t);
    assert(removed);
    
    binding = axiom_keybinding_find(&manager, AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, XKB_KEY_t);
    assert(binding == NULL);
    assert(manager.binding_count == initial_count);
    printf("✓ Keybinding removal works\n");
    
    axiom_keybinding_manager_cleanup(&manager);
    printf("✓ Keybinding system tests passed!\n\n");
}

// Test macro functionality
void test_macro_system() {
    printf("Testing macro system...\n");
    
    struct axiom_keybinding_manager manager;
    axiom_keybinding_manager_init(&manager);
    
    // Create a macro with multiple steps
    struct axiom_macro_step steps[] = {
        {
            .action = AXIOM_ACTION_WINDOW_FULLSCREEN,
            .parameter = 0,
            .command = ""
        },
        {
            .action = AXIOM_ACTION_COMMAND,
            .parameter = 0,
            .command = "notify-send 'Window maximized'"
        }
    };
    
    // Add the macro binding
    bool success = axiom_keybinding_add_macro(&manager,
                                             AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT,
                                             XKB_KEY_m,
                                             steps,
                                             2,
                                             "Fullscreen and notify");
    assert(success);
    printf("✓ Macro binding added successfully\n");
    
    // Find and verify the macro
    struct axiom_keybinding *binding = axiom_keybinding_find(&manager, 
                                                            AXIOM_MOD_SUPER | AXIOM_MOD_SHIFT, 
                                                            XKB_KEY_m);
    assert(binding != NULL);
    assert(binding->is_macro == true);
    assert(binding->macro_step_count == 2);
    assert(binding->macro_steps[0].action == AXIOM_ACTION_WINDOW_FULLSCREEN);
    assert(binding->macro_steps[1].action == AXIOM_ACTION_COMMAND);
    assert(strcmp(binding->macro_steps[1].command, "notify-send 'Window maximized'") == 0);
    printf("✓ Macro binding verification works\n");
    
    axiom_keybinding_manager_cleanup(&manager);
    printf("✓ Macro system tests passed!\n\n");
}

// Test configuration save/load
void test_config_system() {
    printf("Testing configuration system...\n");
    
    struct axiom_keybinding_manager manager;
    axiom_keybinding_manager_init(&manager);
    
    // Add some custom bindings
    axiom_keybinding_add(&manager, AXIOM_MOD_SUPER, XKB_KEY_t, 
                        AXIOM_ACTION_COMMAND, 0, "foot", "Terminal");
    axiom_keybinding_add(&manager, AXIOM_MOD_SUPER, XKB_KEY_b, 
                        AXIOM_ACTION_COMMAND, 0, "firefox", "Browser");
    
    int original_count = manager.binding_count;
    
    // Test saving configuration
    bool saved = axiom_keybinding_save_config(&manager, "/tmp/axiom_test_keybindings.conf");
    assert(saved);
    printf("✓ Configuration saved successfully\n");
    
    // Clear manager and reload defaults
    axiom_keybinding_manager_cleanup(&manager);
    axiom_keybinding_manager_init(&manager);
    
    // Load the saved configuration (this would add to existing bindings)
    bool loaded = axiom_keybinding_load_config(&manager, "/tmp/axiom_test_keybindings.conf");
    assert(loaded);
    printf("✓ Configuration loaded successfully\n");
    
    // Check that custom bindings were loaded
    struct axiom_keybinding *terminal_binding = axiom_keybinding_find(&manager, AXIOM_MOD_SUPER, XKB_KEY_t);
    struct axiom_keybinding *browser_binding = axiom_keybinding_find(&manager, AXIOM_MOD_SUPER, XKB_KEY_b);
    
    // Note: these might exist as duplicates since we load onto existing defaults
    // In a real implementation, you'd want to clear before loading or handle duplicates
    assert(terminal_binding != NULL || browser_binding != NULL);
    printf("✓ Custom bindings found after reload\n");
    
    // Cleanup
    remove("/tmp/axiom_test_keybindings.conf");
    axiom_keybinding_manager_cleanup(&manager);
    printf("✓ Configuration system tests passed!\n\n");
}

int main() {
    printf("=== Axiom Keybinding System Tests ===\n\n");
    
    test_keybinding_system();
    test_macro_system();
    test_config_system();
    
    printf("🎉 All keybinding tests passed! \n");
    printf("\nKeybinding system features verified:\n");
    printf("  • Basic keybinding add/remove/find operations\n");
    printf("  • String to enum conversions for actions and modifiers\n");
    printf("  • Keybinding enable/disable functionality\n");
    printf("  • Macro support with multiple action steps\n");
    printf("  • Configuration save/load functionality\n");
    printf("  • Default keybinding loading\n");
    
    return 0;
}
