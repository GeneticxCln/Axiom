#include "../include/thumbnail_manager.h"
#include "../include/axiom.h"
#include <assert.h>

void test_thumbnail_creation() {
    struct axiom_server server = {0};
    struct axiom_thumbnail_manager *manager = axiom_thumbnail_manager_create(&server);
    assert(manager != NULL);
    axiom_thumbnail_manager_destroy(manager);
    printf("Thumbnail creation test passed.\n");
}

void test_thumbnail_update() {
    struct axiom_server server = {0};
    struct axiom_thumbnail_manager *manager = axiom_thumbnail_manager_create(&server);
    
    // Properly initialize window structure
    struct axiom_window window = {0};
    window.surface = NULL; // No surface for test
    window.xdg_toplevel = NULL;
    window.is_focused = false;
    
    struct axiom_thumbnail *thumbnail = axiom_thumbnail_create(manager, &window);
    assert(thumbnail != NULL);
    
    bool updated = axiom_thumbnail_update(manager, thumbnail);
    assert(updated);
    
    // Test thumbnail data access
    const uint8_t *data = axiom_thumbnail_get_pixel_data(thumbnail);
    assert(data != NULL);
    
    size_t data_size = axiom_thumbnail_get_data_size(thumbnail);
    assert(data_size > 0);
    
    axiom_thumbnail_destroy(thumbnail);
    axiom_thumbnail_manager_destroy(manager);
    printf("Thumbnail update test passed.\n");
}

int main() {
    test_thumbnail_creation();
    test_thumbnail_update();
    printf("All thumbnail tests passed.\n");
    return 0;
}
