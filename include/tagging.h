#ifndef AXIOM_TAGGING_H
#define AXIOM_TAGGING_H

#include <stdint.h>
#include <stdbool.h>

struct axiom_server;
struct axiom_window;

// Tag system constants
#define AXIOM_TAGS_MAX 9
#define AXIOM_TAG_ALL ((1 << AXIOM_TAGS_MAX) - 1)

// Tag structure
struct axiom_tag_manager {
    uint32_t selected_tags;     // Currently visible tags (bitmask)
    uint32_t previous_tags;     // Previous tag selection for toggling
    char *tag_names[AXIOM_TAGS_MAX];  // Custom tag names
    int tag_window_counts[AXIOM_TAGS_MAX];  // Windows per tag
    bool sticky_windows_visible;  // Show sticky windows on all tags
};

// Window tagging properties are now defined in axiom.h

// Tag management functions
void axiom_tag_manager_init(struct axiom_tag_manager *manager);
void axiom_tag_manager_cleanup(struct axiom_tag_manager *manager);

// Tag selection and viewing
void axiom_tag_view(struct axiom_server *server, uint32_t tags);
void axiom_tag_toggle_view(struct axiom_server *server, uint32_t tag);
void axiom_tag_view_all(struct axiom_server *server);
void axiom_tag_view_previous(struct axiom_server *server);

// Window tagging
void axiom_window_set_tags(struct axiom_server *server, struct axiom_window *window, uint32_t tags);
void axiom_window_toggle_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag);
void axiom_window_add_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag);
void axiom_window_remove_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag);

// Window movement between tags
void axiom_window_move_to_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag);
void axiom_focused_window_move_to_tag(struct axiom_server *server, uint32_t tag);

// Sticky windows (visible on all tags)
void axiom_window_set_sticky(struct axiom_server *server, struct axiom_window *window, bool sticky);
void axiom_window_toggle_sticky(struct axiom_server *server, struct axiom_window *window);

// Tag utilities
bool axiom_window_is_visible(struct axiom_window *window, uint32_t selected_tags);
uint32_t axiom_tag_mask_from_number(int tag_number);
int axiom_tag_count_windows(struct axiom_server *server, uint32_t tag);
void axiom_tag_update_counts(struct axiom_server *server);

// Tag naming
void axiom_tag_set_name(struct axiom_tag_manager *manager, int tag_index, const char *name);
const char *axiom_tag_get_name(struct axiom_tag_manager *manager, int tag_index);

// Tag information and debugging
void axiom_tag_print_status(struct axiom_server *server);
const char *axiom_tag_mask_to_string(uint32_t tags);

#endif /* AXIOM_TAGGING_H */
