#!/bin/bash

# Axiom Debug Launcher - Catches crashes and logs
# Usage: ./debug-axiom.sh

echo "🐛 Starting Axiom in Debug Mode..."

# Create debug log directory
mkdir -p /tmp/axiom-debug
LOG_FILE="/tmp/axiom-debug/axiom-$(date +%Y%m%d-%H%M%S).log"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AXIOM_BINARY="$SCRIPT_DIR/build/axiom"

# Check if binary exists
if [ ! -f "$AXIOM_BINARY" ]; then
    echo "❌ Error: Axiom binary not found at $AXIOM_BINARY"
    echo "   Please build first with: meson compile -C build"
    exit 1
fi

# Safety check - don't run if we're in a graphical session
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    echo "⚠️  WARNING: You appear to be running in a graphical session"
    echo "   This should be run from a TTY (Ctrl+Alt+F2)"
    echo ""
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Set environment for better debugging
export WLR_NO_HARDWARE_CURSORS=1
export WLR_RENDERER=gles2
export WAYLAND_DEBUG=1
export WLR_DRM_NO_ATOMIC=1

echo "🖥️  Environment:"
echo "   TTY: $(tty 2>/dev/null || echo 'unknown')"
echo "   USER: $USER"
echo "   GROUPS: $(groups)"
echo "   GPU Access: $(ls -la /dev/dri/ 2>/dev/null | wc -l) devices"
echo ""

echo "📝 Logging to: $LOG_FILE"
echo "🚀 Starting Axiom with crash detection..."

# Function to handle cleanup
cleanup() {
    echo ""
    echo "🛑 Axiom stopped or crashed"
    echo "📊 Debug info:"
    echo "   Exit code: $?"
    echo "   Log file: $LOG_FILE"
    
    if [ -f "$LOG_FILE" ]; then
        echo "   Log size: $(wc -l < "$LOG_FILE") lines"
        echo ""
        echo "📄 Last 20 lines of log:"
        tail -20 "$LOG_FILE"
    fi
    
    echo ""
    echo "🔧 Troubleshooting tips:"
    echo "   1. Check if you're in 'video' group: groups \$USER"
    echo "   2. Try from pure TTY (no X11/Wayland)"
    echo "   3. Check GPU permissions: ls -la /dev/dri/"
    echo "   4. View full log: cat $LOG_FILE"
}

# Set trap to catch exits
trap cleanup EXIT

# Check GPU access
if [ ! -r /dev/dri/card0 ]; then
    echo "❌ No access to GPU. Add user to video group:"
    echo "   sudo usermod -a -G video $USER"
    echo "   Then log out and back in"
    exit 1
fi

# Run axiom with full logging
echo "Starting..." | tee "$LOG_FILE"
exec "$AXIOM_BINARY" 2>&1 | tee -a "$LOG_FILE"
