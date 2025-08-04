#!/bin/bash

# Axiom Window Management Test Script
# Tests actual window management functionality

echo "🪟 Axiom Window Management Test Suite"
echo "====================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check if we're running in a Wayland session (required for nested mode)
if [ -z "$WAYLAND_DISPLAY" ]; then
    echo -e "${RED}❌ Error: Not running in a Wayland session${NC}"
    echo "This test requires a Wayland compositor (like GNOME, KDE, or Hyprland)"
    echo "Start your Wayland session and try again."
    exit 1
fi

echo -e "${GREEN}✓ Running in Wayland session ($WAYLAND_DISPLAY)${NC}"

# Test results
PASSED=0
FAILED=0
WARNINGS=0

test_result() {
    if [ $1 -eq 0 ]; then
        echo -e "  ${GREEN}✓ PASS${NC}: $2"
        ((PASSED++))
    else
        echo -e "  ${RED}✗ FAIL${NC}: $2"
        ((FAILED++))
    fi
}

test_warning() {
    echo -e "  ${YELLOW}⚠ WARN${NC}: $1"
    ((WARNINGS++))
}

echo -e "\n${BLUE}Phase 1: Compositor Startup Test${NC}"
echo "--------------------------------"

# Start Axiom in nested mode
echo "Starting Axiom compositor in nested mode..."
export AXIOM_TEST_MODE=1
./builddir/axiom --nested > /tmp/axiom_wm_test.log 2>&1 &
AXIOM_PID=$!

# Wait for startup
echo "Waiting for compositor to initialize..."
sleep 3

if kill -0 $AXIOM_PID 2>/dev/null; then
    test_result 0 "Compositor started successfully"
    
    # Check if WAYLAND_DISPLAY was set by Axiom
    sleep 1
    AXIOM_DISPLAY=$(grep "socket:" /tmp/axiom_wm_test.log | tail -1 | awk '{print $NF}')
    if [ -n "$AXIOM_DISPLAY" ]; then
        export WAYLAND_DISPLAY="$AXIOM_DISPLAY"
        test_result 0 "Wayland socket created: $AXIOM_DISPLAY" 
    else
        test_warning "Could not detect Axiom's Wayland socket"
    fi
else
    test_result 1 "Compositor failed to start"
    echo "Check logs: /tmp/axiom_wm_test.log"
    exit 1
fi

echo -e "\n${BLUE}Phase 2: Basic Client Connection Test${NC}"
echo "------------------------------------"

# Test basic Wayland client connection
echo "Testing basic client connection..."
if command -v weston-terminal > /dev/null 2>&1; then
    echo "Using weston-terminal for client test..."
    timeout 5s weston-terminal > /dev/null 2>&1 &
    CLIENT_PID=$!
    sleep 2
    
    if kill -0 $CLIENT_PID 2>/dev/null; then
        test_result 0 "Wayland client can connect"
        kill $CLIENT_PID 2>/dev/null
    else
        test_result 1 "Wayland client connection failed"
    fi
elif command -v alacritty > /dev/null 2>&1; then
    echo "Using alacritty for client test..."
    timeout 5s alacritty > /dev/null 2>&1 &
    CLIENT_PID=$!
    sleep 2
    
    if kill -0 $CLIENT_PID 2>/dev/null; then
        test_result 0 "Wayland client can connect"
        kill $CLIENT_PID 2>/dev/null
    else
        test_result 1 "Wayland client connection failed"
    fi
else
    test_warning "No suitable Wayland client found for testing"
fi

echo -e "\n${BLUE}Phase 3: Window Creation Test${NC}"
echo "-----------------------------"

# Test simple window creation with weston-simple-egl if available  
echo "Testing window creation..."
if command -v weston-simple-egl > /dev/null 2>&1; then
    echo "Using weston-simple-egl for window test..."
    timeout 3s weston-simple-egl > /dev/null 2>&1 &
    SIMPLE_PID=$!
    sleep 1
    
    if kill -0 $SIMPLE_PID 2>/dev/null; then
        test_result 0 "Window creation successful"
        kill $SIMPLE_PID 2>/dev/null
        wait $SIMPLE_PID 2>/dev/null
    else
        test_result 1 "Window creation failed"
    fi
else
    test_warning "weston-simple-egl not available for window creation test"
fi

echo -e "\n${BLUE}Phase 4: Log Analysis${NC}"
echo "--------------------"

# Analyze Axiom logs for issues
echo "Analyzing compositor logs..."
if [ -f "/tmp/axiom_wm_test.log" ]; then
    ERROR_COUNT=$(grep -c "ERROR" /tmp/axiom_wm_test.log)
    WARN_COUNT=$(grep -c "WARN" /tmp/axiom_wm_test.log)
    
    if [ $ERROR_COUNT -eq 0 ]; then
        test_result 0 "No errors in compositor logs"
    else
        test_result 1 "Found $ERROR_COUNT errors in logs"
        echo "  Recent errors:"
        grep "ERROR" /tmp/axiom_wm_test.log | tail -3 | sed 's/^/    /'
    fi
    
    if [ $WARN_COUNT -lt 5 ]; then
        test_result 0 "Minimal warnings in logs ($WARN_COUNT warnings)"
    else
        test_warning "Many warnings in logs ($WARN_COUNT warnings)"
    fi
    
    # Check for specific functionality
    if grep -q "initialized" /tmp/axiom_wm_test.log; then
        test_result 0 "Compositor initialization completed"
    else
        test_result 1 "Initialization may have failed"
    fi
    
else
    test_result 1 "No log file found"
fi

echo -e "\n${BLUE}Phase 5: Memory Usage Test${NC}"
echo "-------------------------"

# Check memory usage
echo "Checking memory usage..."
if kill -0 $AXIOM_PID 2>/dev/null; then
    MEMORY_KB=$(ps -o rss= -p $AXIOM_PID 2>/dev/null)
    if [ -n "$MEMORY_KB" ]; then
        MEMORY_MB=$((MEMORY_KB / 1024))
        if [ $MEMORY_MB -lt 100 ]; then
            test_result 0 "Memory usage acceptable (${MEMORY_MB}MB)"
        elif [ $MEMORY_MB -lt 200 ]; then
            test_warning "Memory usage moderate (${MEMORY_MB}MB)"
        else
            test_result 1 "Memory usage high (${MEMORY_MB}MB)"
        fi
    else
        test_warning "Could not measure memory usage"
    fi
fi

echo -e "\n${BLUE}Phase 6: Cleanup Test${NC}"
echo "--------------------"

# Test graceful shutdown
echo "Testing graceful shutdown..."
if kill -0 $AXIOM_PID 2>/dev/null; then
    kill -TERM $AXIOM_PID
    sleep 2
    
    if kill -0 $AXIOM_PID 2>/dev/null; then
        # Force kill if still running
        kill -KILL $AXIOM_PID 2>/dev/null
        test_warning "Compositor required force kill"
    else
        test_result 0 "Compositor shut down gracefully"
    fi
else
    test_result 1 "Compositor already terminated"
fi

# Wait for complete shutdown
wait $AXIOM_PID 2>/dev/null

echo -e "\n${BLUE}Summary${NC}"
echo "======="
echo -e "Tests Passed: ${GREEN}$PASSED${NC}"
echo -e "Tests Failed: ${RED}$FAILED${NC}"
echo -e "Warnings: ${YELLOW}$WARNINGS${NC}"

TOTAL=$((PASSED + FAILED))
if [ $TOTAL -gt 0 ]; then
    PASS_RATE=$((PASSED * 100 / TOTAL))
    echo -e "Pass Rate: ${BLUE}$PASS_RATE%${NC}"
fi

echo -e "\n${BLUE}Assessment:${NC}"
if [ $FAILED -eq 0 ]; then
    if [ $WARNINGS -eq 0 ]; then
        echo -e "${GREEN}🎉 Excellent! Window management system is working well.${NC}"
        echo "Ready for advanced functionality testing."
    else
        echo -e "${YELLOW}✅ Good! Core functionality works with minor issues.${NC}"
        echo "Address warnings for better performance."
    fi
else
    echo -e "${RED}⚠ Issues detected in window management.${NC}"
    echo "Critical fixes needed before proceeding."
fi

echo -e "\n${BLUE}Next Steps:${NC}"
if [ $FAILED -eq 0 ]; then
    echo "1. Test keyboard shortcuts and window focus"
    echo "2. Test multi-window scenarios"
    echo "3. Test workspace switching functionality"
    echo "4. Profile performance under load"
else
    echo "1. Review error logs: /tmp/axiom_wm_test.log"
    echo "2. Fix critical window management issues"
    echo "3. Re-run tests after fixes"
fi

echo -e "\n${BLUE}Log Files:${NC}"
echo "- Main log: /tmp/axiom_wm_test.log"
echo "- View errors: grep ERROR /tmp/axiom_wm_test.log"
echo "- View warnings: grep WARN /tmp/axiom_wm_test.log"

echo -e "\nWindow management test completed at $(date)"
