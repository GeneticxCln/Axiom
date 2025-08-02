#!/bin/bash

# Axiom Compositor Launcher Script
# Usage: ./run-axiom.sh [--nested]

echo "🚀 Starting Axiom Wayland Compositor..."

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AXIOM_BINARY="$SCRIPT_DIR/build/axiom"

# Check if binary exists
if [ ! -f "$AXIOM_BINARY" ]; then
    echo "❌ Error: Axiom binary not found at $AXIOM_BINARY"
    echo "   Please build first with: meson compile -C build"
    exit 1
fi

# Set environment for better compatibility
export WLR_NO_HARDWARE_CURSORS=1
export WLR_RENDERER=gles2

# Check if we should run nested
if [ "$1" = "--nested" ]; then
    echo "🪟 Running in nested mode..."
    exec "$AXIOM_BINARY" --nested
else
    echo "🖥️  Running as main compositor..."
    echo "   Make sure you're in a TTY (not X11/Wayland session)"
    exec "$AXIOM_BINARY"
fi
