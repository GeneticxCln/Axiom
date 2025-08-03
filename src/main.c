#include "compositor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void print_usage(const char *progname) {
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
            fprintf(stderr, "Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    struct axiom_server server = {0};

    if (!axiom_compositor_init(&server, nested)) {
        fprintf(stderr, "Failed to initialize compositor\n");
        return EXIT_FAILURE;
    }

    axiom_compositor_run(&server);

    axiom_compositor_cleanup(&server);

    return EXIT_SUCCESS;
}
