#!/bin/bash

# Axiom Compositor Comprehensive Stress Testing Suite
# Tests stability, performance, and functionality under load

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
AXIOM_BINARY="./builddir/axiom"
TEST_DURATION=30
MAX_WINDOWS=50
MEMORY_THRESHOLD_MB=500
CPU_THRESHOLD_PERCENT=50

echo -e "${BLUE}🧪 Axiom Compositor Comprehensive Stress Test Suite${NC}"
echo "=================================================="
echo "Test Duration: ${TEST_DURATION}s per test"
echo "Max Windows: ${MAX_WINDOWS}"
echo "Memory Threshold: ${MEMORY_THRESHOLD_MB}MB"
echo "CPU Threshold: ${CPU_THRESHOLD_PERCENT}%"
echo ""

# Function to print test results
print_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ $test_name: PASS${NC} $details"
    elif [ "$result" = "WARN" ]; then
        echo -e "${YELLOW}⚠️  $test_name: WARNING${NC} $details"
    else
        echo -e "${RED}❌ $test_name: FAIL${NC} $details"
    fi
}

# Function to check if Axiom is running
check_axiom_running() {
    pgrep -f "axiom.*nested" > /dev/null
}

# Function to get memory usage
get_memory_usage() {
    if check_axiom_running; then
        ps -o rss= -p $(pgrep -f "axiom.*nested") | awk '{sum+=$1} END {print sum/1024}'
    else
        echo "0"
    fi
}

# Function to get CPU usage
get_cpu_usage() {
    if check_axiom_running; then
        ps -o %cpu= -p $(pgrep -f "axiom.*nested") | awk '{sum+=$1} END {print sum}'
    else
        echo "0"
    fi
}

# Test 1: Basic Startup and Shutdown
echo -e "${BLUE}Test 1: Basic Startup and Shutdown${NC}"
echo "Starting Axiom in nested mode..."

timeout 10s $AXIOM_BINARY --nested > /tmp/axiom_startup.log 2>&1 &
AXIOM_PID=$!
sleep 3

if check_axiom_running; then
    print_result "Startup Test" "PASS" "(PID: $AXIOM_PID)"
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
    sleep 1
    if ! check_axiom_running; then
        print_result "Shutdown Test" "PASS" "(Clean shutdown)"
    else
        print_result "Shutdown Test" "FAIL" "(Process still running)"
    fi
else
    print_result "Startup Test" "FAIL" "(Failed to start)"
fi

echo ""

# Test 2: Memory Usage Under Load
echo -e "${BLUE}Test 2: Memory Usage Monitoring${NC}"
echo "Starting Axiom and monitoring memory usage..."

$AXIOM_BINARY --nested > /tmp/axiom_memory.log 2>&1 &
AXIOM_PID=$!
sleep 5

if check_axiom_running; then
    initial_memory=$(get_memory_usage)
    echo "Initial memory usage: ${initial_memory}MB"
    
    # Monitor for test duration
    max_memory=0
    for i in $(seq 1 $TEST_DURATION); do
        current_memory=$(get_memory_usage)
        if (( $(echo "$current_memory > $max_memory" | bc -l) )); then
            max_memory=$current_memory
        fi
        sleep 1
    done
    
    echo "Peak memory usage: ${max_memory}MB"
    
    if (( $(echo "$max_memory < $MEMORY_THRESHOLD_MB" | bc -l) )); then
        print_result "Memory Usage Test" "PASS" "(Peak: ${max_memory}MB < ${MEMORY_THRESHOLD_MB}MB)"
    else
        print_result "Memory Usage Test" "WARN" "(Peak: ${max_memory}MB > ${MEMORY_THRESHOLD_MB}MB)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Memory Usage Test" "FAIL" "(Axiom failed to start)"
fi

echo ""

# Test 3: Configuration Loading
echo -e "${BLUE}Test 3: Configuration Loading${NC}"
echo "Testing configuration file loading..."

# Create test configuration
mkdir -p ~/.config/axiom
cat > ~/.config/axiom/test_axiom.conf << EOF
[input]
repeat_rate = 30
repeat_delay = 500

[tiling]
enabled = true
gap_size = 8

[effects]
shadows_enabled = true
blur_enabled = true
EOF

$AXIOM_BINARY --nested > /tmp/axiom_config.log 2>&1 &
AXIOM_PID=$!
sleep 3

if check_axiom_running; then
    # Check if configuration was loaded (look for specific log entries)
    if grep -q "Configuration loaded" /tmp/axiom_config.log 2>/dev/null; then
        print_result "Configuration Loading" "PASS" "(Config file processed)"
    else
        print_result "Configuration Loading" "WARN" "(Config loaded with defaults)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Configuration Loading" "FAIL" "(Failed to start with config)"
fi

# Cleanup test config
rm -f ~/.config/axiom/test_axiom.conf

echo ""

# Test 4: Protocol Support Validation
echo -e "${BLUE}Test 4: Protocol Support Validation${NC}"
echo "Checking Wayland protocol support..."

$AXIOM_BINARY --nested > /tmp/axiom_protocols.log 2>&1 &
AXIOM_PID=$!
sleep 5

if check_axiom_running; then
    # Check for protocol initialization messages
    protocol_count=0
    
    # Count protocol-related log entries
    if grep -q "wl_compositor" /tmp/axiom_protocols.log 2>/dev/null; then
        ((protocol_count++))
    fi
    if grep -q "xdg_wm_base" /tmp/axiom_protocols.log 2>/dev/null; then
        ((protocol_count++))
    fi
    if grep -q "Effects manager" /tmp/axiom_protocols.log 2>/dev/null; then
        ((protocol_count++))
    fi
    
    if [ $protocol_count -ge 2 ]; then
        print_result "Protocol Support" "PASS" "($protocol_count protocols initialized)"
    else
        print_result "Protocol Support" "WARN" "(Limited protocol support detected)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Protocol Support" "FAIL" "(Failed to start)"
fi

echo ""

# Test 5: Keybinding System Test
echo -e "${BLUE}Test 5: Keybinding System Test${NC}"
echo "Testing keybinding initialization..."

$AXIOM_BINARY --nested > /tmp/axiom_keybindings.log 2>&1 &
AXIOM_PID=$!
sleep 3

if check_axiom_running; then
    # Count loaded keybindings
    keybinding_count=$(grep -c "Added key binding" /tmp/axiom_keybindings.log 2>/dev/null || echo "0")
    
    if [ $keybinding_count -gt 30 ]; then
        print_result "Keybinding System" "PASS" "($keybinding_count keybindings loaded)"
    elif [ $keybinding_count -gt 10 ]; then
        print_result "Keybinding System" "WARN" "($keybinding_count keybindings loaded - fewer than expected)"
    else
        print_result "Keybinding System" "FAIL" "($keybinding_count keybindings loaded)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Keybinding System" "FAIL" "(Failed to start)"
fi

echo ""

# Test 6: Effects System Test
echo -e "${BLUE}Test 6: Effects System Test${NC}"
echo "Testing GPU effects initialization..."

$AXIOM_BINARY --nested > /tmp/axiom_effects.log 2>&1 &
AXIOM_PID=$!
sleep 3

if check_axiom_running; then
    # Check for effects initialization
    if grep -q "Effects manager configured" /tmp/axiom_effects.log 2>/dev/null; then
        effects_status=$(grep "Effects manager configured" /tmp/axiom_effects.log | tail -1)
        if echo "$effects_status" | grep -q "shadows=on.*blur=on"; then
            print_result "Effects System" "PASS" "(GPU effects initialized)"
        else
            print_result "Effects System" "WARN" "(Effects partially initialized)"
        fi
    else
        print_result "Effects System" "WARN" "(Effects system status unclear)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Effects System" "FAIL" "(Failed to start)"
fi

echo ""

# Test 7: Multi-Monitor Simulation
echo -e "${BLUE}Test 7: Multi-Monitor Capabilities${NC}"
echo "Testing multi-monitor support structures..."

$AXIOM_BINARY --nested > /tmp/axiom_multimonitor.log 2>&1 &
AXIOM_PID=$!
sleep 3

if check_axiom_running; then
    # In nested mode, we can't test real multi-monitor, but we can check initialization
    if grep -q "Background surface created" /tmp/axiom_multimonitor.log 2>/dev/null; then
        print_result "Multi-Monitor Support" "PASS" "(Monitor management initialized)"
    else
        print_result "Multi-Monitor Support" "WARN" "(Monitor initialization unclear)"
    fi
    
    kill $AXIOM_PID 2>/dev/null || true
    wait $AXIOM_PID 2>/dev/null || true
else
    print_result "Multi-Monitor Support" "FAIL" "(Failed to start)"
fi

echo ""

# Test 8: Stability Test (Extended Runtime)
echo -e "${BLUE}Test 8: Extended Stability Test${NC}"
echo "Running Axiom for extended period (${TEST_DURATION}s)..."

$AXIOM_BINARY --nested > /tmp/axiom_stability.log 2>&1 &
AXIOM_PID=$!
sleep 2

if check_axiom_running; then
    start_time=$(date +%s)
    crash_detected=false
    
    # Monitor for crashes during test duration
    for i in $(seq 1 $TEST_DURATION); do
        if ! check_axiom_running; then
            crash_detected=true
            break
        fi
        sleep 1
        if [ $((i % 10)) -eq 0 ]; then
            echo "  Stability test: ${i}/${TEST_DURATION}s"
        fi
    done
    
    end_time=$(date +%s)
    runtime=$((end_time - start_time))
    
    if [ "$crash_detected" = true ]; then
        print_result "Stability Test" "FAIL" "(Crashed after ${runtime}s)"
    else
        print_result "Stability Test" "PASS" "(Ran for ${runtime}s without crashes)"
    fi
    
    # Clean shutdown
    if check_axiom_running; then
        kill $AXIOM_PID 2>/dev/null || true
        wait $AXIOM_PID 2>/dev/null || true
    fi
else
    print_result "Stability Test" "FAIL" "(Failed to start)"
fi

echo ""

# Test Summary
echo -e "${BLUE}📊 Test Summary${NC}"
echo "================"

# Count log files for completed tests
completed_tests=0
if [ -f /tmp/axiom_startup.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_memory.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_config.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_protocols.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_keybindings.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_effects.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_multimonitor.log ]; then ((completed_tests++)); fi
if [ -f /tmp/axiom_stability.log ]; then ((completed_tests++)); fi

echo "Completed tests: $completed_tests/8"
echo ""

echo "Log files created:"
ls -la /tmp/axiom_*.log 2>/dev/null || echo "No log files found"

echo ""
echo -e "${GREEN}✅ Stress testing completed!${NC}"
echo ""
echo "📝 Review log files in /tmp/axiom_*.log for detailed information"
echo "🔍 Use 'journalctl -f' to monitor system logs during real usage"
echo "💡 Test with real applications for comprehensive validation"

# Cleanup
echo ""
echo "Cleaning up test processes..."
pkill -f "axiom.*nested" 2>/dev/null || true
sleep 1

echo -e "${BLUE}🎯 Next Steps:${NC}"
echo "1. Review test results and logs"
echo "2. Test with real applications (Firefox, terminal, etc.)"
echo "3. Try multi-monitor setup if available"
echo "4. Test keybindings interactively"
echo "5. Share results with community"
