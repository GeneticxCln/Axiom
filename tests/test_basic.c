#include <assert.h>
#include <stdio.h>
#include <math.h>

// Minimal axiom_server struct for testing
struct axiom_server {
    int workspace_width;
    int workspace_height;
    int window_count;
};

// Copy of the function we want to test
void axiom_calculate_window_layout(struct axiom_server *server, int index, int *x, int *y, int *width, int *height) {
    if (server->workspace_width <= 0 || server->workspace_height <= 0) {
        *x = *y = 0;
        *width = 800;
        *height = 600;
        return;
    }
    
    int window_count = server->window_count;
    if (window_count == 1) {
        *x = 0;
        *y = 0;
        *width = server->workspace_width;
        *height = server->workspace_height;
    } else if (window_count == 2) {
        *width = server->workspace_width / 2;
        *height = server->workspace_height;
        *x = index * (*width);
        *y = 0;
    } else {
        // Grid layout for more than 2 windows
        int cols = (int)ceil(sqrt(window_count));
        int rows = (int)ceil((double)window_count / cols);
        
        *width = server->workspace_width / cols;
        *height = server->workspace_height / rows;
        
        int col = index % cols;
        int row = index / cols;
        
        *x = col * (*width);
        *y = row * (*height);
    }
}

void test_window_layout_calculation() {
    struct axiom_server server = {0};
    server.workspace_width = 1920;
    server.workspace_height = 1080;
    server.window_count = 2;
    
    int x, y, width, height;
    
    // Test first window in 2-window layout
    axiom_calculate_window_layout(&server, 0, &x, &y, &width, &height);
    assert(x == 0);
    assert(y == 0);
    assert(width == 960);
    assert(height == 1080);
    
    // Test second window
    axiom_calculate_window_layout(&server, 1, &x, &y, &width, &height);
    assert(x == 960);
    assert(y == 0);
    assert(width == 960);
    assert(height == 1080);
    
    printf("✓ Window layout calculation tests passed\n");
}

void test_single_window_layout() {
    struct axiom_server server = {0};
    server.workspace_width = 1920;
    server.workspace_height = 1080;
    server.window_count = 1;
    
    int x, y, width, height;
    
    axiom_calculate_window_layout(&server, 0, &x, &y, &width, &height);
    assert(x == 0);
    assert(y == 0);
    assert(width == 1920);
    assert(height == 1080);
    
    printf("✓ Single window layout test passed\n");
}

void test_grid_layout() {
    struct axiom_server server = {0};
    server.workspace_width = 1920;
    server.workspace_height = 1080;
    server.window_count = 4;
    
    int x, y, width, height;
    
    // Test first window (top-left)
    axiom_calculate_window_layout(&server, 0, &x, &y, &width, &height);
    assert(x == 0);
    assert(y == 0);
    assert(width == 960);
    assert(height == 540);
    
    // Test second window (top-right)
    axiom_calculate_window_layout(&server, 1, &x, &y, &width, &height);
    assert(x == 960);
    assert(y == 0);
    assert(width == 960);
    assert(height == 540);
    
    // Test third window (bottom-left)
    axiom_calculate_window_layout(&server, 2, &x, &y, &width, &height);
    assert(x == 0);
    assert(y == 540);
    assert(width == 960);
    assert(height == 540);
    
    printf("✓ Grid layout tests passed\n");
}

int main() {
    test_single_window_layout();
    test_window_layout_calculation();
    test_grid_layout();
    printf("All tests passed!\n");
    return 0;
}
