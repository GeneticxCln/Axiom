#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <wayland-server-core.h>

// Minimal logging macros for testing
#define AXIOM_LOG_INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define AXIOM_LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define AXIOM_LOG_DEBUG(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Include the animation header
#include "animation.h"

// Mock server structure for testing
struct test_server {
    struct axiom_animation_manager *animation_manager;
};

// Test update callback
static void test_update_callback(struct axiom_animation *anim, void *user_data) {
    int *counter = (int*)user_data;
    (*counter)++;
    printf("Animation update: progress=%.2f, callback called %d times\n", 
           anim->progress, *counter);
}

// Test completion callback
static void test_complete_callback(struct axiom_animation *anim __attribute__((unused)), void *user_data) {
    int *completed = (int*)user_data;
    *completed = 1;
    printf("Animation completed!\n");
}

int main(void) {
    printf("Testing Animation System...\n");
    
    // Test 1: Animation manager initialization
    printf("\n1. Testing animation manager initialization...\n");
    struct test_server server = {0};
    axiom_animation_manager_init((struct axiom_server*)&server);
    assert(server.animation_manager != NULL);
    assert(server.animation_manager->enabled == true);
    printf("✓ Animation manager initialized successfully\n");
    
    // Test 2: Animation creation
    printf("\n2. Testing animation creation...\n");
    struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_APPEAR, 1000);
    assert(anim != NULL);
    assert(anim->type == AXIOM_ANIM_WINDOW_APPEAR);
    assert(anim->duration_ms == 1000);
    assert(anim->state == AXIOM_ANIM_STOPPED);
    printf("✓ Animation created successfully\n");
    
    // Test 3: Animation callbacks
    printf("\n3. Testing animation callbacks...\n");
    int update_counter = 0;
    int completed = 0;
    anim->on_update = test_update_callback;
    anim->on_complete = test_complete_callback;
    anim->user_data = &update_counter;
    printf("✓ Animation callbacks set successfully\n");
    
    // Test 4: Easing function
    printf("\n4. Testing easing functions...\n");
    float result = axiom_easing_apply(AXIOM_EASE_LINEAR, 0.5f);
    assert(result == 0.5f);
    printf("✓ Linear easing: t=0.5 -> %.2f\n", result);
    
    result = axiom_easing_apply(AXIOM_EASE_OUT_CUBIC, 0.5f);
    printf("✓ Cubic easing: t=0.5 -> %.2f\n", result);
    
    // Test 5: Animation lifecycle  
    printf("\n5. Testing animation lifecycle...\n");
    
    // Start animation
    axiom_animation_start(server.animation_manager, anim);
    assert(anim->state == AXIOM_ANIM_RUNNING);
    assert(server.animation_manager->active_count == 1);
    printf("✓ Animation started successfully\n");
    
    // Simulate animation updates for a short duration
    uint32_t start_time = anim->start_time_ms;
    for (int i = 0; i < 5; i++) {
        uint32_t current_time = start_time + (i * 200); // 200ms intervals
        axiom_animation_manager_update(server.animation_manager, current_time);
        usleep(50000); // 50ms delay for visual feedback
    }
    
    // Force completion
    uint32_t end_time = start_time + 1100; // Past duration
    anim->on_complete = test_complete_callback;
    anim->user_data = &completed;
    axiom_animation_manager_update(server.animation_manager, end_time);
    
    printf("✓ Animation updates completed, update callback called %d times\n", update_counter);
    
    // Test 6: Cleanup
    printf("\n6. Testing cleanup...\n");
    axiom_animation_manager_cleanup(server.animation_manager);
    printf("✓ Animation manager cleaned up successfully\n");
    
    printf("\n✅ All animation tests passed!\n");
    return 0;
}
