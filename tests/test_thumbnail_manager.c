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
    struct axiom_window window;
    struct axiom_thumbnail *thumbnail = axiom_thumbnail_create(manager, &window);
    assert(thumbnail != NULL);
    bool updated = axiom_thumbnail_update(manager, thumbnail);
    assert(updated);
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
