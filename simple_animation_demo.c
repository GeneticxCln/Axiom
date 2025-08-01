#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include "animation.h"

static void demo_callback(struct axiom_animation *anim, void *data) {
    const char *name = (const char*)data;
    printf("✅ Animation '%s' completed! Final progress: %.2f\n", name, anim->progress);
}

static uint32_t get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int main() {
    printf("🎬 Axiom Animation System Standalone Demo\n");
    printf("==========================================\n\n");
    
    // Initialize animation manager manually since we don't have a server
    struct axiom_animation_manager manager = {0};
    wl_list_init(&manager.animations);
    manager.enabled = true;
    manager.active_count = 0;
    manager.global_speed_multiplier = 1.0f;
    printf("✅ Animation manager initialized\n");
    
    // Create some test animations
    struct axiom_animation *fade_anim = axiom_animation_create(AXIOM_ANIM_FADE, 2000);
    fade_anim->on_complete = demo_callback;
    fade_anim->user_data = "Window Fade";
    
    struct axiom_animation *focus_anim = axiom_animation_create(AXIOM_ANIM_FOCUS_RING, 1500);
    focus_anim->on_complete = demo_callback;
    focus_anim->user_data = "Focus Ring";
    
    struct axiom_animation *scale_anim = axiom_animation_create(AXIOM_ANIM_SCALE, 1000);
    scale_anim->on_complete = demo_callback;
    scale_anim->user_data = "Scale Animation";
    
    printf("✅ Created 3 test animations\n");
    
    // Start animations
    axiom_animation_start(&manager, fade_anim);
    axiom_animation_start(&manager, focus_anim);
    axiom_animation_start(&manager, scale_anim);
    printf("🚀 Started all animations\n\n");
    
    // Run animation loop for demo
    printf("⏳ Running animation loop...\n");
    uint32_t start_time = get_time_ms();
    for (int i = 0; i < 50; i++) {
        uint32_t current_time = get_time_ms();
        axiom_animation_manager_update(&manager, current_time);
        
        // Print progress every 10 iterations
        if (i % 10 == 0) {
            printf("📊 Frame %d (%.1fs) - Active animations: %d\n", i, 
                   (current_time - start_time) / 1000.0f, manager.active_count);
        }
        
        usleep(100000); // 100ms delay
    }
    
    printf("\n🏁 Demo complete\n");
    
    // Cleanup
    axiom_animation_manager_cleanup(&manager);
    printf("✅ Animation manager cleaned up\n");
    
    return 0;
}
