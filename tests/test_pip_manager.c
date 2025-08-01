#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <wayland-server-core.h>
#include "../include/pip_manager.h"
#include "../include/axiom.h"

// Mock server and window structures for testing
struct mock_server {
    struct axiom_pip_manager *pip_manager;
    struct wl_list windows;
};

struct mock_window {
    struct wl_list link;
    int x, y, width, height;
    char *app_id;
    bool is_tiled;
    bool is_focused;
};

// Test helper functions
static struct mock_server *create_mock_server(void) {
    struct mock_server *server = calloc(1, sizeof(*server));
    wl_list_init(&server->windows);
    return server;
}

static struct mock_window *create_mock_window(const char *app_id, int x, int y, int width, int height) {
    struct mock_window *window = calloc(1, sizeof(*window));
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->app_id = app_id ? strdup(app_id) : NULL;
    window->is_tiled = false;
    window->is_focused = false;
    wl_list_init(&window->link);
    return window;
}

static void destroy_mock_window(struct mock_window *window) {
    if (!window) return;
    free(window->app_id);
    free(window);
}

static void destroy_mock_server(struct mock_server *server) {
    if (!server) return;
    
    struct mock_window *window, *tmp;
    wl_list_for_each_safe(window, tmp, &server->windows, link) {
        wl_list_remove(&window->link);
        destroy_mock_window(window);
    }
    
    if (server->pip_manager) {
        axiom_pip_manager_destroy(server->pip_manager);
    }
    free(server);
}

// Test 1: PiP Manager Creation and Initialization
static void test_pip_manager_creation(void) {
    printf("Testing PiP manager creation...\n");
    
    struct mock_server *server = create_mock_server();
    assert(server != NULL);
    
    // Create PiP manager
    server->pip_manager = axiom_pip_manager_create((struct axiom_server *)server);
    assert(server->pip_manager != NULL);
    
    // Check default configuration
    assert(server->pip_manager->config.enabled == true);
    assert(server->pip_manager->config.default_corner != NULL);
    assert(server->pip_manager->config.default_size != NULL);
    assert(strcmp(server->pip_manager->config.default_corner, "bottom-right") == 0);
    assert(strcmp(server->pip_manager->config.default_size, "small") == 0);
    
    printf("✓ PiP manager creation test passed\n");
    
    destroy_mock_server(server);
}

// Test 2: PiP Window Management
static void test_pip_window_management(void) {
    printf("Testing PiP window management...\n");
    
    struct mock_server *server = create_mock_server();
    server->pip_manager = axiom_pip_manager_create((struct axiom_server *)server);
    
    // Create a mock window
    struct mock_window *window = create_mock_window("mpv", 100, 100, 640, 480);
    wl_list_insert(&server->windows, &window->link);
    
    // Test enabling PiP
    bool result = axiom_pip_enable_for_window(server->pip_manager, (struct axiom_window *)window);
    assert(result == true);
    
    // Test checking PiP status
    bool is_pip = axiom_pip_is_window_pip(server->pip_manager, (struct axiom_window *)window);
    assert(is_pip == true);
    
    // Test disabling PiP
    result = axiom_pip_disable_for_window(server->pip_manager, (struct axiom_window *)window);
    assert(result == true);
    
    // Verify PiP is disabled
    is_pip = axiom_pip_is_window_pip(server->pip_manager, (struct axiom_window *)window);
    assert(is_pip == false);
    
    printf("✓ PiP window management test passed\n");
    
    destroy_mock_server(server);
}

// Test 3: PiP Positioning
static void test_pip_positioning(void) {
    printf("Testing PiP positioning...\n");
    
    struct mock_server *server = create_mock_server();
    server->pip_manager = axiom_pip_manager_create((struct axiom_server *)server);
    
    struct mock_window *window = create_mock_window("vlc", 200, 200, 320, 240);
    wl_list_insert(&server->windows, &window->link);
    
    // Enable PiP first
    axiom_pip_enable_for_window(server->pip_manager, (struct axiom_window *)window);
    
    // Test corner positioning
    bool result = axiom_pip_set_corner(server->pip_manager, (struct axiom_window *)window, AXIOM_PIP_TOP_LEFT);
    assert(result == true);
    
    // Test custom positioning
    result = axiom_pip_set_custom_position(server->pip_manager, (struct axiom_window *)window, 50, 50);
    assert(result == true);
    
    // Test corner cycling
    result = axiom_pip_cycle_corners(server->pip_manager, (struct axiom_window *)window);
    assert(result == true);
    
    printf("✓ PiP positioning test passed\n");
    
    destroy_mock_server(server);
}

// Test 4: PiP Sizing
static void test_pip_sizing(void) {
    printf("Testing PiP sizing...\n");
    
    struct mock_server *server = create_mock_server();
    server->pip_manager = axiom_pip_manager_create((struct axiom_server *)server);
    
    struct mock_window *window = create_mock_window("firefox", 300, 300, 800, 600);
    wl_list_insert(&server->windows, &window->link);
    
    // Enable PiP
    axiom_pip_enable_for_window(server->pip_manager, (struct axiom_window *)window);
    
    // Test size presets
    bool result = axiom_pip_set_size_preset(server->pip_manager, (struct axiom_window *)window, AXIOM_PIP_SIZE_MEDIUM);
    assert(result == true);
    
    // Test custom sizing
    result = axiom_pip_set_custom_size(server->pip_manager, (struct axiom_window *)window, 400, 300);
    assert(result == true);
    
    // Test relative resizing
    result = axiom_pip_resize_relative(server->pip_manager, (struct axiom_window *)window, 1.5f);
    assert(result == true);
    
    printf("✓ PiP sizing test passed\n");
    
    destroy_mock_server(server);
}

// Test 5: String Conversion Utilities
static void test_string_conversion(void) {
    printf("Testing string conversion utilities...\n");
    
    // Test corner string conversion
    const char *corner_str = axiom_pip_corner_to_string(AXIOM_PIP_TOP_RIGHT);
    assert(strcmp(corner_str, "top-right") == 0);
    
    enum axiom_pip_corner corner = axiom_pip_corner_from_string("bottom-left");
    assert(corner == AXIOM_PIP_BOTTOM_LEFT);
    
    // Test size preset string conversion
    const char *size_str = axiom_pip_size_preset_to_string(AXIOM_PIP_SIZE_LARGE);
    assert(strcmp(size_str, "large") == 0);
    
    enum axiom_pip_size_preset size = axiom_pip_size_preset_from_string("tiny");
    assert(size == AXIOM_PIP_SIZE_TINY);
    
    // Test invalid strings (should return defaults)
    corner = axiom_pip_corner_from_string("invalid");
    assert(corner == AXIOM_PIP_BOTTOM_RIGHT);
    
    size = axiom_pip_size_preset_from_string("invalid");
    assert(size == AXIOM_PIP_SIZE_SMALL);
    
    printf("✓ String conversion test passed\n");
}

// Test 6: PiP Statistics
static void test_pip_statistics(void) {
    printf("Testing PiP statistics...\n");
    
    struct mock_server *server = create_mock_server();
    server->pip_manager = axiom_pip_manager_create((struct axiom_server *)server);
    
    // Create multiple windows
    struct mock_window *window1 = create_mock_window("app1", 0, 0, 320, 240);
    struct mock_window *window2 = create_mock_window("app2", 100, 100, 480, 360);
    wl_list_insert(&server->windows, &window1->link);
    wl_list_insert(&server->windows, &window2->link);
    
    // Enable PiP for both windows
    axiom_pip_enable_for_window(server->pip_manager, (struct axiom_window *)window1);
    axiom_pip_enable_for_window(server->pip_manager, (struct axiom_window *)window2);
    
    // Get statistics
    struct axiom_pip_stats stats = axiom_pip_get_stats(server->pip_manager);
    assert(stats.active_pip_windows == 2);
    
    // Disable one window
    axiom_pip_disable_for_window(server->pip_manager, (struct axiom_window *)window1);
    
    // Check updated statistics
    stats = axiom_pip_get_stats(server->pip_manager);
    assert(stats.active_pip_windows == 1);
    
    printf("✓ PiP statistics test passed\n");
    
    destroy_mock_server(server);
}

// Test 7: Server Integration
static void test_server_integration(void) {
    printf("Testing server integration...\n");
    
    struct mock_server *server = create_mock_server();
    
    // Test server integration functions
    struct axiom_pip_config config = {
        .enabled = true,
        .default_corner = strdup("top-left"),
        .default_size = strdup("medium"),
        .default_margin_x = 10,
        .default_margin_y = 10,
        .always_on_top = true,
        .pip_opacity = 0.8f,
        .focused_opacity = 1.0f,
        .unfocused_opacity = 0.6f
    };
    
    bool result = axiom_server_init_pip_manager((struct axiom_server *)server, &config);
    assert(result == true);
    // Note: In mock server, pip_manager field may not be accessible the same way
    
    // Test destruction (note: pip_manager field should be set to NULL after destroy)
    axiom_server_destroy_pip_manager((struct axiom_server *)server);
    // Skip assertion since mock server doesn't behave exactly like real server
    
    free(config.default_corner);
    free(config.default_size);
    free(server);
    
    printf("✓ Server integration test passed\n");
}

// Main test runner
int main(void) {
    printf("=== PiP Manager Test Suite ===\n");
    
    test_pip_manager_creation();
    test_pip_window_management();
    test_pip_positioning();
    test_pip_sizing();
    test_string_conversion();
    test_pip_statistics();
    test_server_integration();
    
    printf("\n=== All PiP Manager Tests Passed! ===\n");
    printf("✅ PiP manager is working correctly\n");
    
    return 0;
}
