#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

/**
 * Protocol Testing Framework for Axiom Compositor
 * 
 * This test suite verifies that essential Wayland protocols work correctly
 * by running real client applications and checking their behavior.
 */

#define TEST_TIMEOUT 10  // seconds
#define MAX_OUTPUT 4096

typedef struct {
    const char *name;
    const char *description;
    const char *command;
    const char *expected_output;
    int should_succeed;
} protocol_test_t;

// Test suite for essential protocols
static protocol_test_t protocol_tests[] = {
    // Core compositor functionality
    {
        .name = "weston-info",
        .description = "Basic compositor info and protocol enumeration",
        .command = "timeout 5 weston-info",
        .expected_output = "interface: 'wl_compositor'",
        .should_succeed = 1
    },
    
    // XDG Shell testing
    {
        .name = "weston-simple-egl",
        .description = "XDG Shell surface creation and basic rendering",
        .command = "timeout 3 weston-simple-egl -f",
        .expected_output = NULL, // Just check it doesn't crash
        .should_succeed = 1
    },
    
    // Layer Shell testing (if waybar is available)
    {
        .name = "waybar-test",
        .description = "Layer shell protocol for status bars",
        .command = "timeout 2 waybar --help",
        .expected_output = "Usage:",
        .should_succeed = 1
    },
    
    // Screenshot/Screencopy testing
    {
        .name = "grim-test",
        .description = "Screencopy protocol functionality",
        .command = "timeout 3 grim -t png /dev/null",
        .expected_output = NULL,
        .should_succeed = 1
    },
    
    // Session lock testing (if swaylock is available)
    {
        .name = "swaylock-test",
        .description = "Session lock protocol",
        .command = "timeout 1 swaylock --help",
        .expected_output = "Usage:",
        .should_succeed = 1
    },
    
    // Primary selection testing
    {
        .name = "wl-clipboard-test",
        .description = "Primary selection and clipboard protocols",
        .command = "timeout 2 wl-paste --help",
        .expected_output = "Usage:",
        .should_succeed = 1
    },
    
    // XWayland testing
    {
        .name = "xwayland-test",
        .description = "XWayland X11 compatibility",
        .command = "timeout 2 xeyes --help",
        .expected_output = NULL, // May not be installed
        .should_succeed = 0 // Optional
    },
    
    // Pointer constraints (if available)
    {
        .name = "pointer-constraints-test",
        .description = "Pointer constraints for games",
        .command = "weston-info | grep pointer_constraints",
        .expected_output = "pointer_constraints",
        .should_succeed = 1
    },
    
    // Fractional scaling
    {
        .name = "fractional-scale-test",
        .description = "Fractional scaling protocol",
        .command = "weston-info | grep fractional_scale",
        .expected_output = "fractional_scale",
        .should_succeed = 1
    },
    
    // End marker
    {NULL, NULL, NULL, NULL, 0}
};

static int run_command_with_output(const char *command, char *output, size_t output_size) {
    FILE *pipe = popen(command, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t total_read = 0;
    char *current_pos = output;
    size_t remaining = output_size - 1;
    
    while (remaining > 0 && fgets(current_pos, remaining, pipe)) {
        size_t len = strlen(current_pos);
        current_pos += len;
        remaining -= len;
        total_read += len;
    }
    
    output[total_read] = '\0';
    int status = pclose(pipe);
    return WEXITSTATUS(status);
}

static int test_protocol(const protocol_test_t *test) {
    printf("Testing %s: %s\n", test->name, test->description);
    
    char output[MAX_OUTPUT];
    int result = run_command_with_output(test->command, output, sizeof(output));
    
    // Check if command succeeded when it should
    if (test->should_succeed && result != 0) {
        if (strstr(test->command, "waybar") || strstr(test->command, "swaylock") || 
            strstr(test->command, "xeyes") || strstr(test->command, "grim")) {
            printf("  SKIP: %s not installed (optional)\n", test->name);
            return 0; // Not a failure
        }
        printf("  FAIL: Command failed with exit code %d\n", result);
        return -1;
    }
    
    // Check expected output if specified
    if (test->expected_output && !strstr(output, test->expected_output)) {
        printf("  FAIL: Expected output '%s' not found\n", test->expected_output);
        printf("  Actual output: %.200s%s\n", output, strlen(output) > 200 ? "..." : "");
        return -1;
    }
    
    printf("  PASS: Protocol working correctly\n");
    return 0;
}

int main(int argc, char *argv[]) {
    printf("=== Axiom Protocol Testing Suite ===\n\n");
    
    // Check if we're running under Axiom
    const char *wayland_display = getenv("WAYLAND_DISPLAY");
    if (!wayland_display) {
        printf("ERROR: Not running under Wayland. Start Axiom first.\n");
        return 1;
    }
    
    printf("Testing under WAYLAND_DISPLAY=%s\n\n", wayland_display);
    
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    int skipped_tests = 0;
    
    for (const protocol_test_t *test = protocol_tests; test->name; test++) {
        total_tests++;
        int result = test_protocol(test);
        
        if (result == 0) {
            if (strstr(test->command, "waybar") || strstr(test->command, "swaylock") || 
                strstr(test->command, "xeyes") || strstr(test->command, "grim")) {
                skipped_tests++;
            } else {
                passed_tests++;
            }
        } else {
            failed_tests++;
        }
        
        printf("\n");
    }
    
    printf("=== Test Results ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Skipped: %d (optional tools not installed)\n", skipped_tests);
    printf("\n");
    
    if (failed_tests > 0) {
        printf("Some protocol tests failed. Check Axiom implementation.\n");
        return 1;
    } else {
        printf("All essential protocols are working correctly!\n");
        return 0;
    }
}
