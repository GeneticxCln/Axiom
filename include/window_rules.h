#ifndef AXIOM_WINDOW_RULES_H
#define AXIOM_WINDOW_RULES_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-server-core.h>

// Forward declarations
struct axiom_server;
struct axiom_window;

// Window rule position types
enum axiom_rule_position {
    AXIOM_RULE_POS_NONE,
    AXIOM_RULE_POS_CENTER,
    AXIOM_RULE_POS_TOP_LEFT,
    AXIOM_RULE_POS_TOP_RIGHT,
    AXIOM_RULE_POS_BOTTOM_LEFT,
    AXIOM_RULE_POS_BOTTOM_RIGHT,
    AXIOM_RULE_POS_MAXIMIZED,
    AXIOM_RULE_POS_FULLSCREEN,
    AXIOM_RULE_POS_CUSTOM,      // Use custom x,y coordinates
};

// Window rule size types
enum axiom_rule_size {
    AXIOM_RULE_SIZE_NONE,
    AXIOM_RULE_SIZE_SMALL,      // 400x300
    AXIOM_RULE_SIZE_MEDIUM,     // 800x600
    AXIOM_RULE_SIZE_LARGE,      // 1200x800
    AXIOM_RULE_SIZE_CUSTOM,     // Use custom width,height
};

// Window rule floating behavior
enum axiom_rule_floating {
    AXIOM_RULE_FLOATING_UNSET,
    AXIOM_RULE_FLOATING_FORCE_TILED,
    AXIOM_RULE_FLOATING_FORCE_FLOATING,
};

// Window rule opacity behavior
enum axiom_rule_opacity {
    AXIOM_RULE_OPACITY_UNSET,
    AXIOM_RULE_OPACITY_OPAQUE,
    AXIOM_RULE_OPACITY_TRANSPARENT,
    AXIOM_RULE_OPACITY_CUSTOM,
};

// Individual window rule
struct axiom_window_rule {
    struct wl_list link;
    
    // Rule matching criteria
    char *app_id;               // Application ID (wayland)
    char *class;                // Window class (X11)  
    char *title;                // Window title pattern
    char *instance;             // Window instance name
    
    // Rule actions
    int workspace;              // Target workspace (-1 = no change)
    enum axiom_rule_position position;
    enum axiom_rule_size size;
    enum axiom_rule_floating floating;
    enum axiom_rule_opacity opacity;
    
    // Custom position/size values
    int custom_x, custom_y;
    int custom_width, custom_height;
    float custom_opacity;
    
    // Effects overrides
    bool disable_shadows;
    bool disable_blur;
    bool disable_animations;
    
    // Picture-in-Picture
    bool enable_pip;
    
    // Priority (higher = applied first)
    int priority;
    
    // Debug info
    char *rule_name;
    bool enabled;
};

// Window rules manager
struct axiom_window_rules_manager {
    struct wl_list rules;           // List of axiom_window_rule
    char *rules_file_path;
    bool rules_enabled;
    uint32_t rules_count;
    
    // Statistics
    uint32_t rules_applied;
    uint32_t rules_matched;
    uint32_t rules_failed;
};

// Window rules manager functions
bool axiom_window_rules_manager_init(struct axiom_window_rules_manager *manager);
void axiom_window_rules_manager_destroy(struct axiom_window_rules_manager *manager);
bool axiom_window_rules_load_config(struct axiom_window_rules_manager *manager, 
                                   const char *config_path);
void axiom_window_rules_reload_config(struct axiom_window_rules_manager *manager);

// Rule management
struct axiom_window_rule *axiom_window_rule_create(void);
void axiom_window_rule_destroy(struct axiom_window_rule *rule);
bool axiom_window_rule_set_app_id(struct axiom_window_rule *rule, const char *app_id);
bool axiom_window_rule_set_class(struct axiom_window_rule *rule, const char *class);
bool axiom_window_rule_set_title(struct axiom_window_rule *rule, const char *title);
bool axiom_window_rule_set_instance(struct axiom_window_rule *rule, const char *instance);

// Rule application
bool axiom_window_rules_apply_to_window(struct axiom_window_rules_manager *manager,
                                       struct axiom_window *window);
struct axiom_window_rule *axiom_window_rules_find_matching_rule(
    struct axiom_window_rules_manager *manager,
    struct axiom_window *window);

// Rule matching functions
bool axiom_window_rule_matches_window(struct axiom_window_rule *rule, 
                                     struct axiom_window *window);
bool axiom_window_rule_matches_app_id(struct axiom_window_rule *rule, 
                                     const char *app_id);
bool axiom_window_rule_matches_class(struct axiom_window_rule *rule, 
                                    const char *class);
bool axiom_window_rule_matches_title(struct axiom_window_rule *rule, 
                                    const char *title);

// Window property extraction
const char *axiom_window_get_app_id(struct axiom_window *window);
const char *axiom_window_get_class(struct axiom_window *window);
const char *axiom_window_get_title(struct axiom_window *window);
const char *axiom_window_get_instance(struct axiom_window *window);

// Rule execution
bool axiom_window_rule_apply_workspace(struct axiom_window_rule *rule,
                                      struct axiom_window *window);
bool axiom_window_rule_apply_position(struct axiom_window_rule *rule,
                                     struct axiom_window *window);
bool axiom_window_rule_apply_size(struct axiom_window_rule *rule,
                                 struct axiom_window *window);
bool axiom_window_rule_apply_floating(struct axiom_window_rule *rule,
                                     struct axiom_window *window);
bool axiom_window_rule_apply_opacity(struct axiom_window_rule *rule,
                                    struct axiom_window *window);
bool axiom_window_rule_apply_effects(struct axiom_window_rule *rule,
                                    struct axiom_window *window);

// Configuration parsing
bool axiom_window_rules_parse_config_file(struct axiom_window_rules_manager *manager,
                                         const char *file_path);
bool axiom_window_rules_parse_rule_section(struct axiom_window_rules_manager *manager,
                                          const char *section_name,
                                          const char *section_content);

// Utility functions
enum axiom_rule_position axiom_window_rules_parse_position(const char *position_str);
enum axiom_rule_size axiom_window_rules_parse_size(const char *size_str);
enum axiom_rule_floating axiom_window_rules_parse_floating(const char *floating_str);
enum axiom_rule_opacity axiom_window_rules_parse_opacity(const char *opacity_str);

// Pattern matching utilities
bool axiom_window_rules_pattern_match(const char *pattern, const char *text);
bool axiom_window_rules_glob_match(const char *pattern, const char *text);

// Debug and statistics
void axiom_window_rules_print_statistics(struct axiom_window_rules_manager *manager);
void axiom_window_rules_print_rules(struct axiom_window_rules_manager *manager);
void axiom_window_rules_debug_window_properties(struct axiom_window *window);

// Integration with server
bool axiom_server_init_window_rules(struct axiom_server *server);
void axiom_server_destroy_window_rules(struct axiom_server *server);

#endif // AXIOM_WINDOW_RULES_H
