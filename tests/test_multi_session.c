#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "multi_session.h"
#include "logging.h"

// Mock server structure for testing
struct axiom_server {
    int dummy;
};

void test_multi_session_manager_creation() {
    printf("Testing multi-session manager creation...\n");
    
    struct axiom_server server = {0};
    struct axiom_multi_session_manager *manager = axiom_multi_session_manager_create(&server);
    
    assert(manager != NULL);
    assert(manager->primary_server == &server);
    assert(manager->next_session_id == 1);
    assert(manager->switching_enabled == true);
    assert(manager->max_sessions == AXIOM_MAX_SESSIONS);
    
    axiom_multi_session_manager_destroy(manager);
    printf("✓ Multi-session manager creation test passed\n");
}

void test_user_session_creation() {
    printf("Testing user session creation...\n");
    
    struct axiom_server server = {0};
    struct axiom_multi_session_manager *manager = axiom_multi_session_manager_create(&server);
    
    struct axiom_user_session *session = axiom_user_session_create(
        manager, "testuser", 1000, 1000, AXIOM_SESSION_USER);
    
    assert(session != NULL);
    assert(strcmp(session->username, "testuser") == 0);
    assert(session->uid == 1000);
    assert(session->gid == 1000);
    assert(session->type == AXIOM_SESSION_USER);
    assert(session->state == AXIOM_SESSION_INACTIVE);
    assert(session->session_id == 1);
    
    axiom_multi_session_manager_destroy(manager);
    printf("✓ User session creation test passed\n");
}

void test_session_switching() {
    printf("Testing session switching...\n");
    
    struct axiom_server server = {0};
    struct axiom_multi_session_manager *manager = axiom_multi_session_manager_create(&server);
    
    // Create multiple sessions
    struct axiom_user_session *session1 = axiom_user_session_create(
        manager, "user1", 1001, 1001, AXIOM_SESSION_USER);
    struct axiom_user_session *session2 = axiom_user_session_create(
        manager, "user2", 1002, 1002, AXIOM_SESSION_USER);
    
    assert(session1 != NULL);
    assert(session2 != NULL);
    
    // Test switching to first user
    bool success = axiom_session_switch_to_user(manager, "user1");
    assert(success == true);
    assert(manager->active_session == session1);
    assert(session1->state == AXIOM_SESSION_ACTIVE);
    
    // Test switching to second user
    success = axiom_session_switch_to_user(manager, "user2");
    assert(success == true);
    assert(manager->active_session == session2);
    assert(session2->state == AXIOM_SESSION_ACTIVE);
    assert(session1->state == AXIOM_SESSION_INACTIVE);
    
    // Test switching to non-existent user
    success = axiom_session_switch_to_user(manager, "nonexistent");
    assert(success == false);
    assert(manager->active_session == session2); // Should remain unchanged
    
    axiom_multi_session_manager_destroy(manager);
    printf("✓ Session switching test passed\n");
}

void test_session_finding() {
    printf("Testing session finding functions...\n");
    
    struct axiom_server server = {0};
    struct axiom_multi_session_manager *manager = axiom_multi_session_manager_create(&server);
    
    struct axiom_user_session *session = axiom_user_session_create(
        manager, "findme", 1003, 1003, AXIOM_SESSION_USER);
    
    // Test find by username
    struct axiom_user_session *found = axiom_session_find_by_username(manager, "findme");
    assert(found == session);
    
    found = axiom_session_find_by_username(manager, "notfound");
    assert(found == NULL);
    
    // Test find by ID
    found = axiom_session_find_by_id(manager, session->session_id);
    assert(found == session);
    
    found = axiom_session_find_by_id(manager, 999);
    assert(found == NULL);
    
    axiom_multi_session_manager_destroy(manager);
    printf("✓ Session finding test passed\n");
}

void test_utility_functions() {
    printf("Testing utility functions...\n");
    
    // Test type to string conversion
    assert(strcmp(axiom_session_type_to_string(AXIOM_SESSION_USER), "User") == 0);
    assert(strcmp(axiom_session_type_to_string(AXIOM_SESSION_GREETER), "Greeter") == 0);
    assert(strcmp(axiom_session_type_to_string(AXIOM_SESSION_LOCK), "Lock") == 0);
    assert(strcmp(axiom_session_type_to_string(AXIOM_SESSION_SYSTEM), "System") == 0);
    
    // Test state to string conversion
    assert(strcmp(axiom_session_state_to_string(AXIOM_SESSION_INACTIVE), "Inactive") == 0);
    assert(strcmp(axiom_session_state_to_string(AXIOM_SESSION_ACTIVE), "Active") == 0);
    assert(strcmp(axiom_session_state_to_string(AXIOM_SESSION_LOCKED), "Locked") == 0);
    assert(strcmp(axiom_session_state_to_string(AXIOM_SESSION_SUSPENDED), "Suspended") == 0);
    assert(strcmp(axiom_session_state_to_string(AXIOM_SESSION_SWITCHING), "Switching") == 0);
    
    printf("✓ Utility functions test passed\n");
}

void test_session_validation() {
    printf("Testing session validation...\n");
    
    struct axiom_server server = {0};
    struct axiom_multi_session_manager *manager = axiom_multi_session_manager_create(&server);
    
    struct axiom_user_session *session = axiom_user_session_create(
        manager, "valid", 1004, 1004, AXIOM_SESSION_USER);
    
    assert(axiom_session_is_valid(session) == true);
    assert(axiom_session_is_valid(NULL) == false);
    
    axiom_multi_session_manager_destroy(manager);
    printf("✓ Session validation test passed\n");
}

int main() {
    printf("🧪 Running Axiom Multi-Session Tests\n");
    printf("=====================================\n\n");
    
    test_multi_session_manager_creation();
    test_user_session_creation();
    test_session_switching();
    test_session_finding();
    test_utility_functions();
    test_session_validation();
    
    printf("\n🎉 All multi-session tests passed!\n\n");
    
    return 0;
}
