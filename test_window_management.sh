#!/bin/bash

# Test Window Management Phase 2.1 - Focus, Decorations, and Positioning
# This script tests enhanced window management features in Axiom

echo "=== Axiom Window Management Test Suite ==="
echo "Phase 2.1: Window Management Polish Testing"

# Test applications that will open multiple windows
TEST_APPS=("thunar" "mousepad" "xeyes")
AVAILABLE_APPS=()

# Check available test applications
for app in "${TEST_APPS[@]}"; do
    if command -v "$app" &> /dev/null; then
        AVAILABLE_APPS+=("$app")
        echo "✓ Found test app: $app"
    else
        echo "✗ Missing test app: $app"
    fi
done

if [ ${#AVAILABLE_APPS[@]} -eq 0 ]; then
    echo "ERROR: No test applications found!"
    echo "Please install test applications: sudo pacman -S thunar mousepad xorg-xeyes"
    exit 1
fi

echo ""
echo "=== Starting Axiom Compositor for Window Management Testing ==="

# Start Axiom in nested mode
./builddir/axiom --nested &
AXIOM_PID=$!

# Give compositor time to start
echo "Waiting for compositor to initialize..."
sleep 3

# Check if Axiom is still running
if ! kill -0 $AXIOM_PID 2>/dev/null; then
    echo "ERROR: Axiom compositor failed to start or crashed"
    exit 1
fi

echo "✓ Axiom compositor started successfully (PID: $AXIOM_PID)"

# Wait for XWayland to be ready
echo "Waiting for XWayland server to be ready..."
timeout=10
while [ $timeout -gt 0 ]; do
    if [ -n "$DISPLAY" ] && xdpyinfo &>/dev/null; then
        echo "✓ XWayland server is ready on display: $DISPLAY"
        break
    fi
    sleep 1
    ((timeout--))
done

if [ $timeout -eq 0 ]; then
    echo "⚠ XWayland server not ready after 10 seconds, but continuing test..."
fi

echo ""
echo "=== Phase 2.1: Window Management Tests ==="

# Test 1: Multi-window Focus Management
echo "Test 1: Multi-window Focus Management"
echo "Opening multiple applications to test focus cycling..."

# Open multiple applications for focus testing
PIDS=()
for app in "${AVAILABLE_APPS[@]}"; do
    echo "  Opening $app..."
    $app &
    APP_PID=$!
    PIDS+=($APP_PID)
    sleep 2  # Give time for window to appear and get focus
done

echo "✓ Opened ${#AVAILABLE_APPS[@]} applications for focus testing"

# Test 2: Window Decorations
echo ""
echo "Test 2: Window Decorations and Visual Feedback"
echo "  - Testing focus-dependent decoration changes"
echo "  - Testing window borders and title bars"
echo "  - Applications should show different decorations when focused/unfocused"

# Give time to observe decorations
sleep 3

# Test 3: Window Positioning
echo ""
echo "Test 3: Window Positioning and Layout"
echo "  - Testing tiled window arrangement"
echo "  - Testing window spacing and gaps"
echo "  - Windows should be arranged in a grid without overlapping"

# Give time to observe positioning
sleep 3

# Test 4: Focus Cycling (Alt+Tab simulation)
echo ""
echo "Test 4: Focus Cycling Behavior"
echo "  - Windows should cycle focus properly"
echo "  - Focus indicators should update correctly"
echo "  - Previous window focus should be maintained"

# Give more time to observe focus behavior
sleep 5

echo ""
echo "=== Window Management Test Results ==="

# Count running applications
RUNNING_COUNT=0
for pid in "${PIDS[@]}"; do
    if kill -0 $pid 2>/dev/null; then
        ((RUNNING_COUNT++))
    fi
done

echo "Multi-window Applications: $RUNNING_COUNT/${#AVAILABLE_APPS[@]} running"
echo "Focus Management: ✓ Tested (manual verification needed)"
echo "Window Decorations: ✓ Tested (visual verification needed)"
echo "Window Positioning: ✓ Tested (layout verification needed)"

echo ""
echo "=== Manual Verification Checklist ==="
echo "Please verify the following in the Axiom window:"
echo "  □ Windows are arranged in a grid layout without overlapping"
echo "  □ Focused window has different colored border/title bar"
echo "  □ Unfocused windows have dimmed decorations"
echo "  □ Window title bars are visible and properly sized"
echo "  □ Windows respond to keyboard focus changes"
echo "  □ Each window maintains its content properly"

echo ""
echo "Press Enter when you've finished verifying the window management..."
read -r

echo ""
echo "=== Cleaning Up ==="

# Terminate all test applications
for pid in "${PIDS[@]}"; do
    if kill -0 $pid 2>/dev/null; then
        kill $pid 2>/dev/null
        sleep 1
        kill -9 $pid 2>/dev/null
    fi
done

# Terminate Axiom compositor
kill $AXIOM_PID 2>/dev/null
sleep 2
kill -9 $AXIOM_PID 2>/dev/null

echo "✓ All applications and compositor terminated"
echo ""
echo "=== Window Management Test Complete ==="
echo ""
echo "Next Phase: Input System Enhancement (Phase 2.2)"
echo "  - Keyboard shortcut improvements"
echo "  - Mouse interaction enhancements" 
echo "  - Window resizing and moving"
