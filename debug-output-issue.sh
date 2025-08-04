#!/bin/bash

# Debug script for Axiom compositor output issues
# This script helps diagnose the "Failed to commit output WL-1" error

echo "=== Axiom Compositor Output Debug Script ==="
echo "This script will help diagnose the output commit issue"
echo

# Check system information
echo "1. System Information:"
echo "   OS: $(uname -o) $(uname -r)"
echo "   GPU: $(lspci | grep -i vga | head -1)"
echo "   Display server: ${XDG_SESSION_TYPE:-unknown}"
echo "   Current display: ${DISPLAY:-none}"
echo "   Wayland display: ${WAYLAND_DISPLAY:-none}"
echo

# Check wlroots and dependencies
echo "2. Dependencies Check:"
pkg-config --exists wlroots-0.19 && echo "   wlroots-0.19: $(pkg-config --modversion wlroots-0.19)" || echo "   wlroots-0.19: NOT FOUND"
pkg-config --exists wayland-server && echo "   wayland-server: $(pkg-config --modversion wayland-server)" || echo "   wayland-server: NOT FOUND"
echo

# Check if we're in a graphical session
echo "3. Session Environment:"
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "   Running inside Wayland session - will test nested mode"
    NESTED_MODE="--nested"
elif [ -n "$DISPLAY" ]; then
    echo "   Running inside X11 session - will test nested mode"
    NESTED_MODE="--nested"
else
    echo "   Running in console mode - will test native mode"
    NESTED_MODE=""
fi
echo

# Check build status
echo "4. Build Check:"
if [ -f "./builddir/axiom" ]; then
    echo "   Axiom binary: Found"
    echo "   Binary size: $(du -h ./builddir/axiom | cut -f1)"
    echo "   Build timestamp: $(stat -c %y ./builddir/axiom)"
else
    echo "   ERROR: Axiom binary not found!"
    echo "   Please run: ninja -C builddir"
    exit 1
fi
echo

# Test basic functionality first
echo "5. Running Axiom with enhanced logging..."
echo "   Command: ./builddir/axiom $NESTED_MODE"
echo "   Press Ctrl+C to stop if it hangs"
echo

# Set debug environment
export WLR_DEBUG=1
export AXIOM_DEBUG=1

# Run the compositor with timeout to prevent hanging
timeout 30s ./builddir/axiom $NESTED_MODE 2>&1 | tee /tmp/axiom-debug.log

# Check the results
echo
echo "6. Analysis Results:"
if grep -q "Failed to commit output" /tmp/axiom-debug.log; then
    echo "   ❌ Output commit error detected"
    echo "   Error details:"
    grep -A 3 -B 3 "Failed to commit output" /tmp/axiom-debug.log | sed 's/^/      /'
else
    echo "   ✅ No output commit errors found"
fi

if grep -q "Compositor initialized successfully" /tmp/axiom-debug.log; then
    echo "   ✅ Compositor initialization successful"
else
    echo "   ❌ Compositor initialization failed"
fi

echo
echo "7. Log file saved to: /tmp/axiom-debug.log"
echo "   Use 'cat /tmp/axiom-debug.log' to view full debug output"
echo
echo "8. Common fixes for output commit issues:"
echo "   - Update graphics drivers"
echo "   - Check if running in proper graphics environment"
echo "   - Try different backend (nested vs native)"
echo "   - Verify wlroots version compatibility"
echo

# Suggest next steps
if grep -q "Failed to commit output" /tmp/axiom-debug.log; then
    echo "9. Recommended next steps:"
    echo "   1. Check graphics driver compatibility"
    echo "   2. Try running with WLR_BACKENDS=headless"
    echo "   3. Test with minimal wlroots example"
    echo "   4. Check system journal: journalctl -xe"
fi
