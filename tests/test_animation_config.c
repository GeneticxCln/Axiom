#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "config.h"
#include "animation.h"

// Provide logging function implementation for tests
void axiom_log(const char *level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    printf("[%s] ", level);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void test_animation_config_load() {
    struct axiom_config *config = axiom_config_create();
    if (!config) {
        printf("❌ Failed to create config\n");
        return;
    }

    if (!axiom_config_load(config, "../examples/axiom.conf")) {
        printf("❌ Failed to load config\n");
        axiom_config_destroy(config);
        return;
    }

    // Check some configuration parameters
    printf("Animations Enabled: %s\n", config->animations_enabled ? "true" : "false");
    printf("Window Appear Duration: %d\n", config->window_appear_duration);
    printf("Default Easing: %s\n", config->default_easing);

    // Assuming these are the expected values
    if (config->animations_enabled && config->window_appear_duration == 300 && strcmp(config->default_easing, "ease_out_cubic") == 0) {
        printf("✅ Configuration loading successful\n");
    } else {
        printf("❌ Configuration loading failed\n");
    }

    axiom_config_destroy(config);
}

int main() {
    printf("Running animation config tests...\n");
    test_animation_config_load();
    return 0;
}
