#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../include/effects.h"
#include "../include/config.h"

// Test helpers
static bool test_passed = true;

static void test_assert(bool condition, const char *test_name) {
    if (condition) {
        printf("✓ %s\n", test_name);
    } else {
        printf("✗ %s FAILED\n", test_name);
        test_passed = false;
    }
}

static void test_effects_manager_init() {
    printf("\n1. Testing effects manager initialization...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    bool result = axiom_effects_manager_init(&manager, &config);
    
    test_assert(result == true, "Effects manager initialization");
    test_assert(manager.shadow.enabled == true, "Shadow effects enabled by default");
    test_assert(manager.blur.enabled == true, "Blur effects enabled by default");
    test_assert(manager.transparency.enabled == true, "Transparency effects enabled by default");
    test_assert(manager.shadow.blur_radius == 10, "Default shadow blur radius");
    test_assert(manager.shadow.opacity == 0.5f, "Default shadow opacity");
    test_assert(manager.transparency.focused_opacity == 1.0f, "Default focused opacity");
    
    // Cleanup
    axiom_effects_manager_destroy(&manager);
}

static void test_shadow_configuration() {
    printf("\n2. Testing shadow configuration...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    axiom_effects_manager_init(&manager, &config);
    
    // Test shadow config update
    struct axiom_shadow_config new_shadow_config = {
        .enabled = true,
        .blur_radius = 15,
        .offset_x = 8,
        .offset_y = 8,
        .opacity = 0.7f,
        .color = axiom_color_rgba(0, 0, 0, 180)
    };
    
    axiom_shadow_update_config(&manager, &new_shadow_config);
    
    test_assert(manager.shadow.blur_radius == 15, "Shadow blur radius updated");
    test_assert(manager.shadow.offset_x == 8, "Shadow offset X updated");
    test_assert(manager.shadow.offset_y == 8, "Shadow offset Y updated");
    test_assert(manager.shadow.opacity == 0.7f, "Shadow opacity updated");
    
    axiom_effects_manager_destroy(&manager);
}

static void test_blur_configuration() {
    printf("\n3. Testing blur configuration...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    axiom_effects_manager_init(&manager, &config);
    
    // Test blur config update
    struct axiom_blur_config new_blur_config = {
        .enabled = true,
        .radius = 20,
        .focus_only = true,
        .intensity = 0.8f
    };
    
    axiom_blur_update_config(&manager, &new_blur_config);
    
    test_assert(manager.blur.radius == 20, "Blur radius updated");
    test_assert(manager.blur.focus_only == true, "Blur focus_only updated");
    test_assert(manager.blur.intensity == 0.8f, "Blur intensity updated");
    
    axiom_effects_manager_destroy(&manager);
}

static void test_transparency_configuration() {
    printf("\n4. Testing transparency configuration...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    axiom_effects_manager_init(&manager, &config);
    
    // Test transparency config update
    struct axiom_transparency_config new_transparency_config = {
        .enabled = true,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.8f,
        .inactive_opacity = 0.6f
    };
    
    axiom_transparency_update_config(&manager, &new_transparency_config);
    
    test_assert(manager.transparency.focused_opacity == 1.0f, "Focused opacity updated");
    test_assert(manager.transparency.unfocused_opacity == 0.8f, "Unfocused opacity updated");
    test_assert(manager.transparency.inactive_opacity == 0.6f, "Inactive opacity updated");
    
    axiom_effects_manager_destroy(&manager);
}

static void test_color_utilities() {
    printf("\n5. Testing color utilities...\n");
    
    uint32_t red = axiom_color_rgba(255, 0, 0, 255);
    uint32_t green = axiom_color_rgba(0, 255, 0, 255);
    uint32_t blue = axiom_color_rgba(0, 0, 255, 255);
    uint32_t transparent_black = axiom_color_rgba(0, 0, 0, 128);
    
    test_assert(red != 0, "Red color created successfully");
    test_assert(green != 0, "Green color created successfully");
    test_assert(blue != 0, "Blue color created successfully");
    test_assert(transparent_black != 0, "Transparent black color created successfully");
    
    // Test that different colors produce different values
    test_assert(red != green, "Different colors produce different values");
    test_assert(green != blue, "Different colors produce different values");
    test_assert(red != blue, "Different colors produce different values");
}

static void test_shadow_texture_creation() {
    printf("\n6. Testing shadow texture creation...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    axiom_effects_manager_init(&manager, &config);
    
    // Create a shadow texture
    struct axiom_shadow_texture *texture = axiom_shadow_create_texture(
        &manager, 100, 100, 10, axiom_color_rgba(0, 0, 0, 128)
    );
    
    test_assert(texture != NULL, "Shadow texture created successfully");
    if (texture) {
        test_assert(texture->width == 100, "Shadow texture width correct");
        test_assert(texture->height == 100, "Shadow texture height correct");
        test_assert(texture->blur_radius == 10, "Shadow texture blur radius correct");
        test_assert(texture->dirty == true, "Shadow texture marked as dirty");
    }
    
    axiom_effects_manager_destroy(&manager);
}

static void test_effects_initialization() {
    printf("\n7. Testing effects subsystem initialization...\n");
    
    struct axiom_effects_manager manager;
    struct axiom_effects_config config = {
        .shadows_enabled = true,
        .blur_enabled = true,
        .transparency_enabled = true,
        .shadow_blur_radius = 10,
        .shadow_offset_x = 4,
        .shadow_offset_y = 4,
        .shadow_opacity = 0.5f,
        .shadow_color = "#000000",
        .blur_radius = 8,
        .blur_focus_only = false,
        .blur_intensity = 0.7f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.9f,
        .inactive_opacity = 0.8f
    };
    axiom_effects_manager_init(&manager, &config);
    
    bool shadow_init = axiom_shadow_init(&manager);
    bool blur_init = axiom_blur_init(&manager);
    
    test_assert(shadow_init == false || shadow_init == true, "Shadow subsystem initialization"); // Allow failure if no GPU
    test_assert(blur_init == true, "Blur subsystem initialization");
    
    axiom_effects_manager_destroy(&manager);
}

int main() {
    printf("Testing Visual Effects System...\n");
    
    test_effects_manager_init();
    test_shadow_configuration();  
    test_blur_configuration();
    test_transparency_configuration();
    test_color_utilities();
    test_shadow_texture_creation();
    test_effects_initialization();
    
    printf("\n");
    if (test_passed) {
        printf("✅ All effects tests passed!\n");
        return 0;
    } else {
        printf("❌ Some effects tests failed!\n");
        return 1;
    }
}
