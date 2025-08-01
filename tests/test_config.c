#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Mock axiom_log function for testing
void axiom_log(const char *level, const char *format, ...) {
    (void)level;
    (void)format;
    // Do nothing in tests
}

// Include the config header and implementation
#include "config.h"
#include "../src/config.c"

void test_config_creation() {
    struct axiom_config *config = axiom_config_create();
    assert(config != NULL);
    
    // Check defaults
    assert(config->cursor_size == 24);
    assert(config->repeat_rate == 25);
    assert(config->repeat_delay == 600);
    assert(config->tiling_enabled == true);
    assert(config->border_width == 2);
    assert(config->gap_size == 5);
    assert(strcmp(config->cursor_theme, "default") == 0);
    assert(strcmp(config->background_color, "#1e1e1e") == 0);
    assert(strcmp(config->border_active, "#ffffff") == 0);
    assert(strcmp(config->border_inactive, "#666666") == 0);
    
    axiom_config_destroy(config);
    printf("✓ Config creation test passed\n");
}

void test_config_file_loading() {
    // Create a temporary config file
    const char *test_config = "/tmp/test_axiom.conf";
    FILE *f = fopen(test_config, "w");
    assert(f != NULL);
    
    fprintf(f, "# Test configuration\n");
    fprintf(f, "[input]\n");
    fprintf(f, "cursor_size = 32\n");
    fprintf(f, "repeat_rate = 30\n");
    fprintf(f, "cursor_theme = \"Adwaita\"\n");
    fprintf(f, "\n");
    fprintf(f, "[tiling]\n");
    fprintf(f, "enabled = false\n");
    fprintf(f, "border_width = 4\n");
    fprintf(f, "gap_size = 10\n");
    fprintf(f, "\n");
    fprintf(f, "[appearance]\n");
    fprintf(f, "background_color = \"#000000\"\n");
    fprintf(f, "border_active = \"#ff0000\"\n");
    
    fclose(f);
    
    struct axiom_config *config = axiom_config_create();
    assert(config != NULL);
    
    bool result = axiom_config_load(config, test_config);
    assert(result == true);
    
    // Check that values were loaded correctly
    assert(config->cursor_size == 32);
    assert(config->repeat_rate == 30);
    assert(config->tiling_enabled == false);
    assert(config->border_width == 4);
    assert(config->gap_size == 10);
    assert(strcmp(config->cursor_theme, "Adwaita") == 0);
    assert(strcmp(config->background_color, "#000000") == 0);
    assert(strcmp(config->border_active, "#ff0000") == 0);
    
    axiom_config_destroy(config);
    unlink(test_config);
    
    printf("✓ Config file loading test passed\n");
}

void test_config_nonexistent_file() {
    struct axiom_config *config = axiom_config_create();
    assert(config != NULL);
    
    // Should return true (not an error) but use defaults
    bool result = axiom_config_load(config, "/nonexistent/path/config.conf");
    assert(result == true);
    
    // Should still have default values
    assert(config->cursor_size == 24);
    assert(config->tiling_enabled == true);
    
    axiom_config_destroy(config);
    printf("✓ Nonexistent config file test passed\n");
}

int main() {
    test_config_creation();
    test_config_file_loading();
    test_config_nonexistent_file();
    printf("All configuration tests passed!\n");
    return 0;
}
