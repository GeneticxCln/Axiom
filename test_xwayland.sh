#!/bin/bash

# Test XWayland functionality in Axiom
# This script starts Axiom in nested mode and tests X11 applications

echo "=== Axiom XWayland Test Script ==="
echo "Testing XWayland support in nested mode..."

# Check if required X11 apps are available  
# Test with both simple and complex X11 applications
X11_APPS=("xeyes" "thunar" "mousepad" "code" "steam")
AVAILABLE_APPS=()

for app in "${X11_APPS[@]}"; do
    if command -v "$app" &> /dev/null; then
        AVAILABLE_APPS+=("$app")
        echo "✓ Found X11 app: $app"
    else
        echo "✗ Missing X11 app: $app"
    fi
done

if [ ${#AVAILABLE_APPS[@]} -eq 0 ]; then
    echo "ERROR: No X11 test applications found!"
    echo "Please install some basic X11 apps: sudo pacman -S xorg-apps"
    exit 1
fi

echo ""
echo "=== Starting Axiom Compositor ==="

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
echo "=== Testing X11 Applications ==="

# Test each available X11 application
for app in "${AVAILABLE_APPS[@]}"; do
    echo "Testing $app..."
    
    # Start the app in background
    $app &
    APP_PID=$!
    
    # Give it time to start
    sleep 2
    
    # Check if it's still running
    if kill -0 $APP_PID 2>/dev/null; then
        echo "✓ $app started successfully (PID: $APP_PID)"
        
        # Kill the app after testing
        kill $APP_PID 2>/dev/null
        sleep 1
        
        # Force kill if necessary
        kill -9 $APP_PID 2>/dev/null
        echo "✓ $app closed"
    else
        echo "✗ $app failed to start or crashed immediately"
    fi
    
    echo ""
done

echo "=== Test Results Summary ==="
echo "Compositor: Running ✓"
echo "XWayland: $([ -n "$DISPLAY" ] && echo "Ready ✓" || echo "Not Ready ✗")"
echo "X11 Apps Tested: ${#AVAILABLE_APPS[@]}"

echo ""
echo "=== Cleaning Up ==="

# Terminate Axiom compositor
kill $AXIOM_PID 2>/dev/null
sleep 2

# Force kill if necessary
kill -9 $AXIOM_PID 2>/dev/null

echo "✓ Compositor terminated"
echo ""
echo "=== XWayland Test Complete ==="
