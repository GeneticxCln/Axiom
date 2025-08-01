#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

struct axiom_config *axiom_config_create(void) {
    struct axiom_config *config = calloc(1, sizeof(struct axiom_config));
    if (!config) {
        return NULL;
    }

    // Set defaults
    config->cursor_theme = strdup("default");
    config->cursor_size = 24;
    config->repeat_rate = 25;
    config->repeat_delay = 600;
    config->tiling_enabled = true;
    config->border_width = 2;
    config->gap_size = 5;
    config->background_color = strdup("#1e1e1e");
    config->border_active = strdup("#ffffff");
    config->border_inactive = strdup("#666666");

    return config;
}

void axiom_config_destroy(struct axiom_config *config) {
    if (!config) {
        return;
    }

    free(config->cursor_theme);
    free(config->background_color);
    free(config->border_active);
    free(config->border_inactive);
    free(config);
}

static void trim_whitespace(char *str) {
    if (!str) return;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) {
        memmove(str, str + 1, strlen(str));
    }
    
    // All spaces?
    if (*str == 0) return;
    
    // Trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

bool axiom_config_load(struct axiom_config *config, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        AXIOM_LOG_INFO("Config file not found at %s, using defaults", path);
        return true; // Not an error, just use defaults
    }

    char line[256];
    char section[64] = "";
    
    while (fgets(line, sizeof(line), file)) {
        trim_whitespace(line);
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        
        // Check for section headers
        if (line[0] == '[' && line[strlen(line)-1] == ']') {
            strncpy(section, line + 1, sizeof(section) - 1);
            section[strlen(section) - 1] = '\0'; // Remove closing bracket
            continue;
        }
        
        // Parse key=value pairs
        char *equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        
        trim_whitespace(key);
        trim_whitespace(value);
        
        // Remove quotes from value if present
        if (value[0] == '"' && value[strlen(value)-1] == '"') {
            value[strlen(value)-1] = '\0';
            value++;
        }
        
        // Parse based on section
        if (strcmp(section, "input") == 0) {
            if (strcmp(key, "repeat_rate") == 0) {
                config->repeat_rate = atoi(value);
            } else if (strcmp(key, "repeat_delay") == 0) {
                config->repeat_delay = atoi(value);
            } else if (strcmp(key, "cursor_theme") == 0) {
                free(config->cursor_theme);
                config->cursor_theme = strdup(value);
            } else if (strcmp(key, "cursor_size") == 0) {
                config->cursor_size = atoi(value);
            }
        } else if (strcmp(section, "tiling") == 0) {
            if (strcmp(key, "enabled") == 0) {
                config->tiling_enabled = strcmp(value, "true") == 0;
            } else if (strcmp(key, "border_width") == 0) {
                config->border_width = atoi(value);
            } else if (strcmp(key, "gap_size") == 0) {
                config->gap_size = atoi(value);
            }
        } else if (strcmp(section, "appearance") == 0) {
            if (strcmp(key, "background_color") == 0) {
                free(config->background_color);
                config->background_color = strdup(value);
            } else if (strcmp(key, "border_active") == 0) {
                free(config->border_active);
                config->border_active = strdup(value);
            } else if (strcmp(key, "border_inactive") == 0) {
                free(config->border_inactive);
                config->border_inactive = strdup(value);
            }
        }
    }
    
    fclose(file);
    AXIOM_LOG_INFO("Configuration loaded from %s", path);
    return true;
}
