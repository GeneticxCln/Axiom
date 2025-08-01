#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "axiom.h"

#define MAX_WORKSPACES 10

// Default workspace names for Phase 2
static const char *default_workspace_names[] = {
    "Main", "Web", "Code", "Term", "Media", 
    "Files", "Chat", "Game", "Misc", "Temp"
};

void axiom_init_workspaces(struct axiom_server *server) {
    server->max_workspaces = MAX_WORKSPACES;
    server->current_workspace = 0;
    
    // Allocate workspace array
    server->workspaces = calloc(MAX_WORKSPACES, sizeof(struct axiom_workspace));
    if (!server->workspaces) {
        AXIOM_LOG_ERROR("Failed to allocate workspaces");
        return;
    }
    
    // Initialize each workspace
    for (int i = 0; i < MAX_WORKSPACES; i++) {
        wl_list_init(&server->workspaces[i].windows);
        server->workspaces[i].width = server->workspace_width;
        server->workspaces[i].height = server->workspace_height;
        
        // Phase 2: Initialize workspace names and settings
        server->workspaces[i].name = strdup(default_workspace_names[i]);
        server->workspaces[i].window_count = 0;
        server->workspaces[i].persistent_layout = false;
        server->workspaces[i].saved_layout_type = 1; // AXIOM_LAYOUT_MASTER_STACK
        server->workspaces[i].saved_master_ratio = 0.6f;
    }
    
    AXIOM_LOG_INFO("Initialized %d workspaces", MAX_WORKSPACES);
}

static void axiom_hide_workspace_windows(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    struct axiom_window *window;
    wl_list_for_each(window, &server->workspaces[workspace].windows, link) {
        // Hide window by setting scene node to disabled
        wlr_scene_node_set_enabled(&window->scene_tree->node, false);
        
        // Hide decorations too
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, false);
        }
    }
}

static void axiom_show_workspace_windows(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    struct axiom_window *window;
    wl_list_for_each(window, &server->workspaces[workspace].windows, link) {
        // Show window by enabling scene node
        wlr_scene_node_set_enabled(&window->scene_tree->node, true);
        
        // Show decorations too
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, true);
        }
    }
}

void axiom_switch_workspace(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        AXIOM_LOG_ERROR("Invalid workspace index: %d", workspace);
        return;
    }
    
    if (workspace == server->current_workspace) {
        AXIOM_LOG_DEBUG("Already on workspace %d", workspace);
        return;
    }
    
    AXIOM_LOG_INFO("Switching from workspace %d to %d", server->current_workspace, workspace);
    
    // Hide current workspace windows
    axiom_hide_workspace_windows(server, server->current_workspace);
    
    // Update current workspace
    // int old_workspace = server->current_workspace; // Currently unused
    server->current_workspace = workspace;
    
    // Show new workspace windows
    axiom_show_workspace_windows(server, workspace);
    
    // Update window count for tiling calculations
    server->window_count = 0;
    struct axiom_window *window;
    wl_list_for_each(window, &server->workspaces[workspace].windows, link) {
        if (window->is_tiled) {
            server->window_count++;
        }
    }
    
    // Re-arrange windows on the new workspace
    if (server->tiling_enabled && server->window_count > 0) {
        axiom_arrange_windows(server);
    }
    
    // Clear focus since we switched workspaces
    server->focused_window = NULL;
    
    AXIOM_LOG_INFO("Switched to workspace %d (contains %d tiled windows)", 
                   workspace, server->window_count);
}

void axiom_move_window_to_workspace(struct axiom_server *server, struct axiom_window *window, int workspace) {
    if (!window || workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    // Find current workspace of the window
    int current_ws = -1;
    for (int i = 0; i < server->max_workspaces; i++) {
        struct axiom_window *w;
        wl_list_for_each(w, &server->workspaces[i].windows, link) {
            if (w == window) {
                current_ws = i;
                break;
            }
        }
        if (current_ws != -1) break;
    }
    
    if (current_ws == -1) {
        AXIOM_LOG_ERROR("Window not found in any workspace");
        return;
    }
    
    if (current_ws == workspace) {
        AXIOM_LOG_DEBUG("Window already in workspace %d", workspace);
        return;
    }
    
    // Remove from current workspace
    wl_list_remove(&window->link);
    
    // Add to target workspace
    wl_list_insert(&server->workspaces[workspace].windows, &window->link);
    
    // Hide window if moving to non-current workspace
    if (workspace != server->current_workspace) {
        wlr_scene_node_set_enabled(&window->scene_tree->node, false);
        if (window->decoration_tree) {
            wlr_scene_node_set_enabled(&window->decoration_tree->node, false);
        }
    }
    
    // Update window counts
    if (current_ws == server->current_workspace && window->is_tiled) {
        server->window_count--;
    }
    
    AXIOM_LOG_INFO("Moved window to workspace %d", workspace);
    
    // Re-arrange current workspace if needed
    if (current_ws == server->current_workspace && server->tiling_enabled) {
        axiom_arrange_windows(server);
    }
}

int axiom_get_workspace_window_count(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return 0;
    }
    
    int count = 0;
    struct axiom_window *window;
    wl_list_for_each(window, &server->workspaces[workspace].windows, link) {
        if (window->is_tiled) {
            count++;
        }
    }
    
    return count;
}

void axiom_cleanup_workspaces(struct axiom_server *server) {
    if (server->workspaces) {
        // Clean up workspace names
        for (int i = 0; i < server->max_workspaces; i++) {
            free(server->workspaces[i].name);
        }
        free(server->workspaces);
        server->workspaces = NULL;
    }
}

// Phase 2: Advanced workspace management functions
void axiom_switch_to_workspace_by_number(struct axiom_server *server, int number) {
    if (number < 1 || number > server->max_workspaces) {
        AXIOM_LOG_ERROR("Invalid workspace number: %d", number);
        return;
    }
    
    // Save current workspace layout before switching
    axiom_save_workspace_layout(server, server->current_workspace);
    
    // Switch to the target workspace (convert 1-based to 0-based)
    axiom_switch_workspace(server, number - 1);
    
    // Restore layout for the new workspace if persistent
    if (server->workspaces[number - 1].persistent_layout) {
        axiom_restore_workspace_layout(server, number - 1);
    }
    
    AXIOM_LOG_INFO("Switched to workspace %d (%s)", number, 
                   axiom_get_workspace_name(server, number - 1));
}

void axiom_move_focused_window_to_workspace(struct axiom_server *server, int workspace) {
    if (!server->focused_window) {
        AXIOM_LOG_INFO("No focused window to move");
        return;
    }
    
    // Convert 1-based workspace number to 0-based index
    int target_ws = workspace - 1;
    if (target_ws < 0 || target_ws >= server->max_workspaces) {
        AXIOM_LOG_ERROR("Invalid workspace number: %d", workspace);
        return;
    }
    
    axiom_move_window_to_workspace(server, server->focused_window, target_ws);
    AXIOM_LOG_INFO("Moved focused window to workspace %d (%s)", workspace,
                   axiom_get_workspace_name(server, target_ws));
}

void axiom_set_workspace_name(struct axiom_server *server, int workspace, const char *name) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    free(server->workspaces[workspace].name);
    server->workspaces[workspace].name = strdup(name ? name : default_workspace_names[workspace]);
    
    AXIOM_LOG_INFO("Set workspace %d name to: %s", workspace + 1, 
                   server->workspaces[workspace].name);
}

const char* axiom_get_workspace_name(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return "Invalid";
    }
    
    return server->workspaces[workspace].name ? 
           server->workspaces[workspace].name : 
           default_workspace_names[workspace];
}

void axiom_save_workspace_layout(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    // Save current layout settings for this workspace
    extern float master_ratio;
    
    server->workspaces[workspace].saved_layout_type = axiom_get_layout();
    server->workspaces[workspace].saved_master_ratio = master_ratio;
    server->workspaces[workspace].persistent_layout = true;
    
    AXIOM_LOG_DEBUG("Saved layout for workspace %d", workspace + 1);
}

void axiom_restore_workspace_layout(struct axiom_server *server, int workspace) {
    if (workspace < 0 || workspace >= server->max_workspaces) {
        return;
    }
    
    if (!server->workspaces[workspace].persistent_layout) {
        return;
    }
    
    // Restore layout settings for this workspace
    extern float master_ratio;
    
    axiom_set_layout(server->workspaces[workspace].saved_layout_type);
    master_ratio = server->workspaces[workspace].saved_master_ratio;
    
    AXIOM_LOG_DEBUG("Restored layout for workspace %d", workspace + 1);
}

int axiom_get_current_workspace(struct axiom_server *server) {
    return server->current_workspace + 1; // Return 1-based workspace number
}

void axiom_update_workspace_indicators(struct axiom_server *server) {
    // Update workspace indicators - this could update title bars or status displays
    AXIOM_LOG_DEBUG("Current workspace: %d (%s)", 
                    axiom_get_current_workspace(server),
                    axiom_get_workspace_name(server, server->current_workspace));
    
    // Future: Update waybar or other status indicators here
    for (int i = 0; i < server->max_workspaces; i++) {
        int window_count = axiom_get_workspace_window_count(server, i);
        if (window_count > 0) {
            AXIOM_LOG_DEBUG("Workspace %d (%s): %d windows", i + 1,
                           axiom_get_workspace_name(server, i), window_count);
        }
    }
}
