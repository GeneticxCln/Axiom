#!/bin/bash

# Debug script to find the black screen issue
echo "=== Axiom Debug Session ==="
echo "Testing Axiom with maximum debug output"
echo ""

# Kill any existing axiom processes
pkill -f axiom

echo "Starting Axiom in nested mode with full debug..."
echo "This will show exactly where the issue occurs"
echo ""

# Run with maximum debugging
WAYLAND_DEBUG=1 \
WLR_NO_HARDWARE_CURSORS=1 \
WLR_RENDERER=pixman \
WLR_BACKENDS=wayland \
MESA_DEBUG=1 \
axiom --nested
