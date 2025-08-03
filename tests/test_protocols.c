#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Protocol Testing Framework for Axiom Compositor
 * 
 * This test suite verifies that essential Wayland protocol headers
 * are available and properly structured for compilation.
 */

typedef struct {
    const char *name;
    const char *description;
    const char *header_file;
    const char *expected_interface;
} protocol_test_t;

// Test suite for essential protocol headers
static protocol_test_t protocol_tests[] = {
    {
        .name = "wayland-server",
        .description = "Core Wayland server protocol",
        .header_file = "protocols/wayland-server-protocol.h",
        .expected_interface = "wl_compositor_interface"
    },
    {
        .name = "xdg-shell",
        .description = "XDG Shell protocol for window management",
        .header_file = "protocols/xdg-shell-protocol.h", 
        .expected_interface = "xdg_wm_base_interface"
    },
    {
        .name = "wlr-layer-shell",
        .description = "Layer shell protocol for panels and bars",
        .header_file = "protocols/wlr-layer-shell-unstable-v1-protocol.h",
        .expected_interface = "zwlr_layer_shell_v1_interface"
    },
    {
        .name = "linux-dmabuf",
        .description = "Linux DMA-BUF protocol for hardware acceleration",
        .header_file = "protocols/linux-dmabuf-v1-protocol.h",
        .expected_interface = "zwp_linux_dmabuf_v1_interface"
    },
    {
        .name = "ext-session-lock",
        .description = "Session lock protocol for screen locking",
        .header_file = "protocols/ext-session-lock-v1-protocol.h",
        .expected_interface = "ext_session_lock_manager_v1_interface"
    },
    {
        .name = "wlr-screencopy",
        .description = "Screencopy protocol for screenshots",
        .header_file = "protocols/wlr-screencopy-unstable-v1-protocol.h",
        .expected_interface = "zwlr_screencopy_manager_v1_interface"
    },
    {
        .name = "fractional-scale",
        .description = "Fractional scaling protocol for HiDPI",
        .header_file = "protocols/fractional-scale-v1-protocol.h",
        .expected_interface = "wp_fractional_scale_manager_v1_interface"
    },
    {
        .name = "xwayland-shell",
        .description = "XWayland shell protocol for X11 compatibility",
        .header_file = "protocols/xwayland-shell-v1-protocol.h",
        .expected_interface = "xwayland_shell_v1_interface"
    },
    // End marker
    {NULL, NULL, NULL, NULL}
};

static int file_exists(const char *filepath) {
    struct stat st;
    return (stat(filepath, &st) == 0);
}

static int test_protocol_header(const protocol_test_t *test) {
    printf("Testing %s: %s\n", test->name, test->description);
    
    // Check if header file exists
    if (!file_exists(test->header_file)) {
        printf("  FAIL: Header file %s not found\n", test->header_file);
        return -1;
    }
    printf("  PASS: Header file %s exists\n", test->header_file);
    
    // Check if expected interface symbol is defined in header
    char command[512];
    snprintf(command, sizeof(command), "grep -q '%s' %s", test->expected_interface, test->header_file);
    
    int result = system(command);
    if (result != 0) {
        printf("  FAIL: Expected interface '%s' not found in header\n", test->expected_interface);
        return -1;
    }
    printf("  PASS: Interface '%s' found in header\n", test->expected_interface);
    
    return 0;
}

int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
    printf("=== Axiom Protocol Header Testing Suite ===\n\n");
    printf("Testing protocol header availability and structure...\n\n");
    
    // Change to source directory to find protocols
    if (chdir("..") != 0) {
        printf("Warning: Could not change to source directory\n");
    }
    
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    
    for (const protocol_test_t *test = protocol_tests; test->name; test++) {
        total_tests++;
        int result = test_protocol_header(test);
        
        if (result == 0) {
            passed_tests++;
        } else {
            failed_tests++;
        }
        
        printf("\n");
    }
    
    printf("=== Test Results ===\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("\n");
    
    if (failed_tests > 0) {
        printf("Some protocol headers are missing or malformed.\n");
        return 1;
    } else {
        printf("All protocol headers are available and properly structured!\n");
        return 0;
    }
}
