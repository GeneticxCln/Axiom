#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "axiom.h"
#include "tagging.h"

// Default tag names
static const char *default_tag_names[AXIOM_TAGS_MAX] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9"
};

void axiom_tag_manager_init(struct axiom_tag_manager *manager) {
    if (!manager) return;
    
    // Initialize with first tag selected
    manager->selected_tags = 1; // Tag 1 (0x001)
    manager->previous_tags = 1;
    manager->sticky_windows_visible = true;
    
    // Initialize tag names
    for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
        manager->tag_names[i] = strdup(default_tag_names[i]);
        manager->tag_window_counts[i] = 0;
    }
    
    AXIOM_LOG_INFO("Tag manager initialized with %d tags", AXIOM_TAGS_MAX);
}

void axiom_tag_manager_cleanup(struct axiom_tag_manager *manager) {
    if (!manager) return;
    
    for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
        free(manager->tag_names[i]);
        manager->tag_names[i] = NULL;
    }
}

void axiom_tag_view(struct axiom_server *server, uint32_t tags) {
    if (!server || !server->tag_manager) return;
    
    if (tags == 0 || tags == server->tag_manager->selected_tags) {
        return; // No change needed
    }
    
    // Save previous selection for toggle
    server->tag_manager->previous_tags = server->tag_manager->selected_tags;
    server->tag_manager->selected_tags = tags;
    
    // Update window visibility
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        bool should_be_visible = axiom_window_is_visible(window, tags);
        
        // Show/hide window based on tag visibility
        wlr_scene_node_set_enabled(&window->scene_tree->node, should_be_visible);
        
        // Show/hide decorations
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, should_be_visible);
        }
    }
    
    // Update window counts and re-arrange visible windows
    axiom_tag_update_counts(server);
    if (server->tiling_enabled) {
        axiom_arrange_windows(server);
    }
    
    AXIOM_LOG_INFO("Viewing tags: %s", axiom_tag_mask_to_string(tags));
}

void axiom_tag_toggle_view(struct axiom_server *server, uint32_t tag) {
    if (!server || !server->tag_manager) return;
    
    uint32_t new_tags = server->tag_manager->selected_tags ^ tag;
    
    // Don't allow viewing no tags
    if (new_tags == 0) {
        new_tags = tag;
    }
    
    axiom_tag_view(server, new_tags);
}

void axiom_tag_view_all(struct axiom_server *server) {
    axiom_tag_view(server, AXIOM_TAG_ALL);
}

void axiom_tag_view_previous(struct axiom_server *server) {
    if (!server || !server->tag_manager) return;
    
    uint32_t prev = server->tag_manager->previous_tags;
    axiom_tag_view(server, prev);
}

void axiom_window_set_tags(struct axiom_server *server, struct axiom_window *window, uint32_t tags) {
    if (!window || !window->window_tags) return;
    
    // Don't allow windows with no tags
    if (tags == 0) {
        tags = 1; // Default to tag 1
    }
    
    window->window_tags->tags = tags;
    
    // Update visibility based on current tag selection
    if (server && server->tag_manager) {
        bool should_be_visible = axiom_window_is_visible(window, server->tag_manager->selected_tags);
        wlr_scene_node_set_enabled(&window->scene_tree->node, should_be_visible);
        
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, should_be_visible);
        }
        
        axiom_tag_update_counts(server);
    }
    
    AXIOM_LOG_INFO("Window tagged with: %s", axiom_tag_mask_to_string(tags));
}

void axiom_window_toggle_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag) {
    if (!window || !window->window_tags) return;
    
    uint32_t new_tags = window->window_tags->tags ^ tag;
    
    // Don't allow windows with no tags
    if (new_tags == 0) {
        new_tags = tag;
    }
    
    axiom_window_set_tags(server, window, new_tags);
}

void axiom_window_add_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag) {
    if (!window || !window->window_tags) return;
    
    uint32_t new_tags = window->window_tags->tags | tag;
    axiom_window_set_tags(server, window, new_tags);
}

void axiom_window_remove_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag) {
    if (!window || !window->window_tags) return;
    
    uint32_t new_tags = window->window_tags->tags & ~tag;
    axiom_window_set_tags(server, window, new_tags);
}

void axiom_window_move_to_tag(struct axiom_server *server, struct axiom_window *window, uint32_t tag) {
    axiom_window_set_tags(server, window, tag);
}

void axiom_focused_window_move_to_tag(struct axiom_server *server, uint32_t tag) {
    if (!server->focused_window) {
        AXIOM_LOG_INFO("No focused window to move");
        return;
    }
    
    axiom_window_move_to_tag(server, server->focused_window, tag);
}

void axiom_window_set_sticky(struct axiom_server *server, struct axiom_window *window, bool sticky) {
    if (!window || !window->window_tags) return;
    
    window->window_tags->is_sticky = sticky;
    
    // Update visibility
    if (server && server->tag_manager) {
        bool should_be_visible = axiom_window_is_visible(window, server->tag_manager->selected_tags);
        wlr_scene_node_set_enabled(&window->scene_tree->node, should_be_visible);
        
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, should_be_visible);
        }
    }
    
    AXIOM_LOG_INFO("Window %s sticky", sticky ? "is now" : "is no longer");
}

void axiom_window_toggle_sticky(struct axiom_server *server, struct axiom_window *window) {
    if (!window || !window->window_tags) return;
    
    axiom_window_set_sticky(server, window, !window->window_tags->is_sticky);
}

bool axiom_window_is_visible(struct axiom_window *window, uint32_t selected_tags) {
    if (!window || !window->window_tags) return false;
    
    // Sticky windows are always visible
    if (window->window_tags->is_sticky) {
        return true;
    }
    
    // Window is visible if any of its tags match selected tags
    return (window->window_tags->tags & selected_tags) != 0;
}

uint32_t axiom_tag_mask_from_number(int tag_number) {
    if (tag_number < 1 || tag_number > AXIOM_TAGS_MAX) {
        return 0;
    }
    return 1 << (tag_number - 1);
}

int axiom_tag_count_windows(struct axiom_server *server, uint32_t tag) {
    if (!server) return 0;
    
    int count = 0;
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (window->window_tags && (window->window_tags->tags & tag)) {
            count++;
        }
    }
    
    return count;
}

void axiom_tag_update_counts(struct axiom_server *server) {
    if (!server || !server->tag_manager) return;
    
    // Reset counts
    for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
        server->tag_manager->tag_window_counts[i] = 0;
    }
    
    // Count windows per tag
    struct axiom_window *window;
    wl_list_for_each(window, &server->windows, link) {
        if (!window->window_tags) continue;
        
        for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
            uint32_t tag = 1 << i;
            if (window->window_tags->tags & tag) {
                server->tag_manager->tag_window_counts[i]++;
            }
        }
    }
    
    // Update server window count for tiling (only visible tiled windows)
    server->window_count = 0;
    wl_list_for_each(window, &server->windows, link) {
        if (window->is_tiled && axiom_window_is_visible(window, server->tag_manager->selected_tags)) {
            server->window_count++;
        }
    }
}

void axiom_tag_set_name(struct axiom_tag_manager *manager, int tag_index, const char *name) {
    if (!manager || tag_index < 0 || tag_index >= AXIOM_TAGS_MAX) return;
    
    free(manager->tag_names[tag_index]);
    manager->tag_names[tag_index] = strdup(name ? name : default_tag_names[tag_index]);
    
    AXIOM_LOG_INFO("Tag %d renamed to: %s", tag_index + 1, manager->tag_names[tag_index]);
}

const char *axiom_tag_get_name(struct axiom_tag_manager *manager, int tag_index) {
    if (!manager || tag_index < 0 || tag_index >= AXIOM_TAGS_MAX) {
        return "Invalid";
    }
    
    return manager->tag_names[tag_index] ? manager->tag_names[tag_index] : default_tag_names[tag_index];
}

void axiom_tag_print_status(struct axiom_server *server) {
    if (!server || !server->tag_manager) return;
    
    AXIOM_LOG_INFO("=== Tag Status ===");
    AXIOM_LOG_INFO("Selected tags: %s", axiom_tag_mask_to_string(server->tag_manager->selected_tags));
    
    for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
        uint32_t tag = 1 << i;
        bool is_selected = (server->tag_manager->selected_tags & tag) != 0;
        int window_count = server->tag_manager->tag_window_counts[i];
        
        AXIOM_LOG_INFO("Tag %d (%s): %d windows %s", 
                       i + 1, 
                       axiom_tag_get_name(server->tag_manager, i),
                       window_count,
                       is_selected ? "[VISIBLE]" : "");
    }
}

const char *axiom_tag_mask_to_string(uint32_t tags) {
    static char buffer[32];
    buffer[0] = '\0';
    
    if (tags == 0) {
        strcpy(buffer, "none");
        return buffer;
    }
    
    if (tags == AXIOM_TAG_ALL) {
        strcpy(buffer, "all");
        return buffer;
    }
    
    bool first = true;
    for (int i = 0; i < AXIOM_TAGS_MAX; i++) {
        if (tags & (1 << i)) {
            if (!first) {
                strcat(buffer, ",");
            }
            char tag_str[8];
            snprintf(tag_str, sizeof(tag_str), "%d", i + 1);
            strcat(buffer, tag_str);
            first = false;
        }
    }
    
    return buffer;
}
