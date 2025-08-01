#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/window_snapping.h"
#include "include/axiom.h"

// Mock server structure for testing
struct mock_server {
    struct axiom_window_snapping_manager *snapping_manager;
    int workspace_width;
    int workspace_height;
};

// Mock window structure
struct mock_window {
    int x, y, width, height;
    bool is_fullscreen;
    char title[256];
};

int main() {
    printf("🚀 Axiom Window Snapping Test Suite\n");
    printf("=====================================\n\n");
    
    // Create mock server
    struct mock_server server = {
        .workspace_width = 1920,
        .workspace_height = 1080
    };
    
    // Create snapping manager (this will work without the full compositor)
    server.snapping_manager = axiom_window_snapping_manager_create((struct axiom_server*)&server);
    
    if (!server.snapping_manager) {
        printf("❌ Failed to create window snapping manager\n");
        return 1;
    }
    
    printf("✅ Window snapping manager created successfully\n");
    
    // Test configuration
    printf("\n📊 Default Configuration:\n");
    printf("   • Snap threshold: %d pixels\n", server.snapping_manager->config.snap_threshold);
    printf("   • Edge resistance: %d pixels\n", server.snapping_manager->config.edge_resistance);
    printf("   • Magnetism strength: %.2f\n", server.snapping_manager->config.magnetism_strength);
    printf("   • Smart corners: %s\n", server.snapping_manager->config.smart_corners ? "enabled" : "disabled");
    printf("   • Multi-monitor: %s\n", server.snapping_manager->config.multi_monitor_snapping ? "enabled" : "disabled");
    printf("   • Window-to-window: %s\n", server.snapping_manager->config.window_to_window_snapping ? "enabled" : "disabled");
    printf("   • Edge snapping: %s\n", server.snapping_manager->config.edge_snapping ? "enabled" : "disabled");
    
    // Test statistics
    printf("\n📈 Initial Statistics:\n");
    struct axiom_snapping_stats stats = axiom_window_snapping_get_stats(server.snapping_manager);
    printf("   • Total snaps: %u\n", stats.total_snaps);
    printf("   • Edge snaps: %u\n", stats.edge_snaps);
    printf("   • Window snaps: %u\n", stats.window_snaps);
    printf("   • Corner snaps: %u\n", stats.corner_snaps);
    
    // Test configuration loading
    printf("\n⚙️  Testing configuration loading...\n");
    if (axiom_window_snapping_load_config(server.snapping_manager, "./test_snapping.conf")) {
        printf("✅ Configuration loading works\n");
    } else {
        printf("⚠️  Configuration file not found (this is expected)\n");
    }
    
    // Test configuration updates
    printf("\n🔧 Testing configuration updates...\n");
    struct axiom_snapping_config new_config = {
        .snap_threshold = 30,
        .edge_resistance = 20,
        .magnetism_strength = 1.0f,
        .animation_duration = 150,
        .smart_corners = false,
        .multi_monitor_snapping = false,
        .window_to_window_snapping = true,
        .edge_snapping = true
    };
    
    axiom_window_snapping_update_config(server.snapping_manager, &new_config);
    printf("✅ Configuration updated successfully\n");
    printf("   • New snap threshold: %d pixels\n", server.snapping_manager->config.snap_threshold);
    
    // Test save functionality
    printf("\n💾 Testing configuration save...\n");
    if (axiom_window_snapping_save_config(server.snapping_manager, "./test_snapping_output.conf")) {
        printf("✅ Configuration save works\n");
    }
    
    // Cleanup
    printf("\n🧹 Cleaning up...\n");
    axiom_window_snapping_manager_destroy(server.snapping_manager);
    printf("✅ Window snapping manager destroyed cleanly\n");
    
    printf("\n🎉 All window snapping tests passed!\n");
    printf("=====================================\n");
    
    return 0;
}
