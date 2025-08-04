#!/bin/bash

# Axiom Compositor Keyboard Shortcut Test Script
# Tests various keybindings and window management features

echo "=== Axiom Compositor Keyboard Shortcut Test ==="
echo "Compositor PID: $(pgrep axiom)"
echo "Active clients: $(pgrep -l weston | wc -l)"
echo ""

# Check if we have the expected test environment
if ! pgrep axiom > /dev/null; then
    echo "ERROR: Axiom compositor is not running!"
    exit 1
fi

echo "Current Wayland clients:"
pgrep -l weston
echo ""

# Function to test a keybinding
test_keybinding() {
    local key_combo="$1"
    local description="$2"
    local expected_result="$3"
    
    echo "Testing: $key_combo - $description"
    echo "Expected: $expected_result"
    echo "Press the key combination now, then press Enter to continue..."
    read -r
    echo ""
}

# Function to check window focus
check_window_focus() {
    echo "=== Window Focus Test ==="
    echo "This test requires manual verification of window focus changes"
    echo ""
    
    test_keybinding "Alt+Tab" "Cycle through windows forward" "Focus should move to next window"
    test_keybinding "Alt+Shift+Tab" "Cycle through windows backward" "Focus should move to previous window"
}

# Function to test window management
test_window_management() {
    echo "=== Window Management Test ==="
    echo ""
    
    test_keybinding "Super+Q" "Close focused window" "Current window should close"
    test_keybinding "Super+Enter" "Open terminal" "New terminal window should appear"
    test_keybinding "Super+F" "Toggle fullscreen" "Window should toggle fullscreen mode"
    test_keybinding "Super+M" "Maximize window" "Window should maximize"
}

# Function to test workspace management
test_workspace_management() {
    echo "=== Workspace Management Test ==="
    echo ""
    
    test_keybinding "Super+1" "Switch to workspace 1" "Should switch to workspace 1"
    test_keybinding "Super+2" "Switch to workspace 2" "Should switch to workspace 2"
    test_keybinding "Super+Shift+1" "Move window to workspace 1" "Window should move to workspace 1"
    test_keybinding "Super+Shift+2" "Move window to workspace 2" "Window should move to workspace 2"
}

# Function to test system keybindings
test_system_keybindings() {
    echo "=== System Keybindings Test ==="
    echo ""
    
    test_keybinding "Super+L" "Lock screen" "Screen should lock (if implemented)"
    test_keybinding "Super+Shift+Q" "Quit compositor" "Compositor should exit gracefully"
}

# Function to monitor compositor logs during testing
monitor_logs() {
    echo "=== Monitoring Compositor Logs ==="
    echo "Checking for any keybinding-related log messages..."
    echo ""
    
    # Since we can't easily tail the logs in nested mode, we'll check the process status
    if pgrep axiom > /dev/null; then
        echo "✓ Compositor is still running"
    else
        echo "✗ Compositor has stopped!"
    fi
    
    echo "Current memory usage:"
    ps -o pid,rss,vsz,comm -p $(pgrep axiom) 2>/dev/null || echo "Could not get memory info"
    echo ""
}

# Main test sequence
main() {
    echo "Starting keyboard shortcut tests..."
    echo "Make sure the Axiom compositor window has focus for testing."
    echo ""
    
    monitor_logs
    
    echo "Select test to run:"
    echo "1) Window Focus Test (Alt+Tab)"
    echo "2) Window Management Test"
    echo "3) Workspace Management Test"
    echo "4) System Keybindings Test"
    echo "5) All Tests"
    echo "6) Monitor Only"
    echo ""
    
    read -p "Enter choice (1-6): " choice
    
    case $choice in
        1) check_window_focus ;;
        2) test_window_management ;;
        3) test_workspace_management ;;
        4) test_system_keybindings ;;
        5) 
            check_window_focus
            test_window_management
            test_workspace_management
            test_system_keybindings
            ;;
        6) monitor_logs ;;
        *) echo "Invalid choice" ;;
    esac
    
    echo ""
    echo "=== Test Complete ==="
    monitor_logs
}

# Run the tests
main
