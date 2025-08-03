#include "compositor.h"
#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void print_usage(const char *progname) {
    // Use direct printf for usage info as it's intentional user output
    printf("Usage: %s [--nested] [--help]\n", progname);
    printf("Options:\n");
    printf("  --nested      Run the compositor in nested mode\n");
    printf("  --help, -h    Show this help message\n");
}

int main(int argc, char *argv[]) {
    bool nested = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--nested") == 0) {
            nested = true;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            AXIOM_LOG_ERROR("Unknown argument: %s", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    struct axiom_server server = {0};

    if (!axiom_compositor_init(&server, nested)) {
        AXIOM_LOG_ERROR("Failed to initialize compositor");
        return EXIT_FAILURE;
    }

    AXIOM_LOG_INFO("Compositor initialized successfully");

    axiom_compositor_run(&server);

    AXIOM_LOG_INFO("Compositor run completed, cleaning up");

    axiom_compositor_cleanup(&server);

    return EXIT_SUCCESS;
}
