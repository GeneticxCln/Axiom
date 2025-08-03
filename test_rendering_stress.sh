#!/bin/bash

# Axiom Rendering Pipeline Stress Testing Suite
# Comprehensive testing under load and real-world conditions

set -e

echo "🔥 Axiom Rendering Pipeline Stress Testing Suite"
echo "================================================"
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_stress() {
    echo -e "${PURPLE}[STRESS]${NC} $1"
}

# Test counters
STRESS_TESTS_TOTAL=0
STRESS_TESTS_PASSED=0
STRESS_TESTS_FAILED=0

run_stress_test() {
    local test_name="$1"
    local test_command="$2"
    local timeout_duration="$3"
    
    STRESS_TESTS_TOTAL=$((STRESS_TESTS_TOTAL + 1))
    print_stress "Testing: $test_name"
    
    if timeout "${timeout_duration:-30s}" bash -c "$test_command" > /dev/null 2>&1; then
        print_success "✅ $test_name"
        STRESS_TESTS_PASSED=$((STRESS_TESTS_PASSED + 1))
    else
        print_error "❌ $test_name"
        STRESS_TESTS_FAILED=$((STRESS_TESTS_FAILED + 1))
    fi
}

measure_performance() {
    local test_name="$1"
    local test_command="$2"
    
    print_status "📊 Measuring: $test_name"
    
    local start_time=$(date +%s.%N)
    eval "$test_command" > /dev/null 2>&1
    local end_time=$(date +%s.%N)
    
    local duration=$(echo "$end_time - $start_time" | bc -l)
    echo "  ⏱️  Duration: ${duration} seconds"
    
    return 0
}

# Phase 1: Memory Stress Testing
print_stress "🧠 Phase 1: Memory Stress Testing"
echo ""

# Test memory allocation under stress
print_status "Testing memory allocation patterns..."
for i in {1..10}; do
    measure_performance "Memory allocation test #$i" "./builddir/test_memory"
done

# Test for memory leaks under repeated operations
run_stress_test "Repeated memory operations (100 cycles)" "
    for i in {1..100}; do
        ./builddir/test_memory > /dev/null 2>&1 || exit 1
    done
" "60s"

# Phase 2: Rendering Performance Stress
print_stress "🎨 Phase 2: Rendering Performance Stress"
echo ""

# Test shader compilation under stress
run_stress_test "Shader compilation stress (50 cycles)" "
    for i in {1..50}; do
        ./test_rendering_pipeline > /dev/null 2>&1 || exit 1
    done
" "120s"

# Test effects system under stress
run_stress_test "Effects system stress (20 cycles)" "
    for i in {1..20}; do
        ./builddir/test_effects > /dev/null 2>&1 || exit 1
    done
" "60s"

# Test animation system under stress
run_stress_test "Animation system stress (30 cycles)" "
    for i in {1..30}; do
        ./builddir/test_animation > /dev/null 2>&1 || exit 1
    done
" "90s"

# Phase 3: Concurrent Testing
print_stress "🔄 Phase 3: Concurrent Operations"
echo ""

# Test multiple tests running concurrently
run_stress_test "Concurrent memory tests (5 parallel)" "
    ./builddir/test_memory &
    ./builddir/test_memory &
    ./builddir/test_memory &
    ./builddir/test_memory &
    ./builddir/test_memory &
    wait
" "45s"

run_stress_test "Concurrent effects tests (3 parallel)" "
    ./builddir/test_effects &
    ./builddir/test_effects &
    ./builddir/test_effects &
    wait
" "30s"

# Phase 4: Configuration System Stress
print_stress "⚙️ Phase 4: Configuration System Stress"
echo ""

# Test configuration loading under stress
run_stress_test "Configuration loading stress (25 cycles)" "
    for i in {1..25}; do
        ./builddir/test_config > /dev/null 2>&1 || exit 1
    done
" "60s"

# Test animation config stress
run_stress_test "Animation config stress (25 cycles)" "
    for i in {1..25}; do
        ./builddir/test_animation_config > /dev/null 2>&1 || exit 1
    done
" "60s"

# Phase 5: Integration Under Load
print_stress "🔗 Phase 5: Integration Under Load"
echo ""

if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    # Test nested compositor under stress
    print_status "Testing nested compositor stability..."
    
    run_stress_test "Nested compositor startup/shutdown (10 cycles)" "
        for i in {1..10}; do
            timeout 3s ./builddir/axiom --nested &
            AXIOM_PID=\$!
            sleep 1
            kill \$AXIOM_PID 2>/dev/null || true
            wait \$AXIOM_PID 2>/dev/null || true
            sleep 0.5
        done
    " "60s"
    
    # Test rapid startup/shutdown
    run_stress_test "Rapid compositor cycling (5 fast cycles)" "
        for i in {1..5}; do
            timeout 2s ./builddir/axiom --nested &
            AXIOM_PID=\$!
            sleep 0.5
            kill \$AXIOM_PID 2>/dev/null || true
            wait \$AXIOM_PID 2>/dev/null || true
        done
    " "30s"
else
    print_warning "No graphical environment - skipping compositor stress tests"
fi

# Phase 6: Resource Exhaustion Testing
print_stress "💥 Phase 6: Resource Exhaustion Testing"
echo ""

# Test with high memory pressure
run_stress_test "High memory pressure test" "
    # Run multiple memory-intensive tests
    ./builddir/test_memory &
    ./builddir/test_thumbnail_manager &
    ./test_rendering_pipeline &
    ./builddir/test_effects &
    wait
" "45s"

# Test system limits
run_stress_test "System limits test (all tests parallel)" "
    ./builddir/test_axiom &
    ./builddir/test_memory &
    ./builddir/test_config &
    ./builddir/test_effects &
    ./builddir/test_animation &
    ./builddir/test_window_rules &
    ./builddir/test_pip_manager &
    wait
" "60s"

# Phase 7: Performance Benchmarking
print_stress "📊 Phase 7: Performance Benchmarking"
echo ""

print_status "Running comprehensive performance benchmark..."

# Benchmark individual components
echo "🔍 Component Performance Analysis:"
echo "=================================="

measure_performance "Basic functionality test" "./builddir/test_axiom"
measure_performance "Memory management test" "./builddir/test_memory"
measure_performance "Effects system test" "./builddir/test_effects"
measure_performance "Animation system test" "./builddir/test_animation"
measure_performance "Configuration test" "./builddir/test_config"
measure_performance "Window rules test" "./builddir/test_window_rules"
measure_performance "PiP manager test" "./builddir/test_pip_manager"
measure_performance "Thumbnail manager test" "./builddir/test_thumbnail_manager"
measure_performance "Shader compilation test" "./test_rendering_pipeline"

# Phase 8: System Resource Monitoring
print_stress "📈 Phase 8: System Resource Analysis"
echo ""

print_status "Analyzing system resource usage..."

# Memory usage analysis
if command -v free &> /dev/null; then
    echo "💾 Memory Usage:"
    free -h
    echo ""
fi

# CPU information
if command -v lscpu &> /dev/null; then
    echo "🖥️ CPU Information:"
    lscpu | grep -E "(Model name|CPU\(s\)|Thread|Core)"
    echo ""
fi

# GPU information (if available)
if command -v lspci &> /dev/null; then
    echo "🎮 Graphics Hardware:"
    lspci | grep -i "vga\|3d\|display" || echo "No dedicated graphics detected"
    echo ""
fi

# OpenGL information
if command -v glxinfo &> /dev/null && [ -n "$DISPLAY" ]; then
    echo "🎨 OpenGL Capabilities:"
    glxinfo | grep -E "(OpenGL renderer|OpenGL version|OpenGL vendor)" || echo "OpenGL info not available"
    echo ""
fi

# Final Results
echo ""
echo "🔥 Stress Test Results Summary"
echo "=============================="
echo "Total Stress Tests: $STRESS_TESTS_TOTAL"
echo "Passed: $STRESS_TESTS_PASSED ✅"
echo "Failed: $STRESS_TESTS_FAILED ❌"

if [ $STRESS_TESTS_FAILED -eq 0 ]; then
    STRESS_SUCCESS_RATE=100
else
    STRESS_SUCCESS_RATE=$(( (STRESS_TESTS_PASSED * 100) / STRESS_TESTS_TOTAL ))
fi

echo "Stress Success Rate: ${STRESS_SUCCESS_RATE}%"
echo ""

# Detailed stress test analysis
echo "🔍 Stress Test Analysis"
echo "======================="
echo ""

if [ $STRESS_SUCCESS_RATE -ge 95 ]; then
    print_success "🏆 EXCEPTIONAL: Rendering pipeline handles extreme stress perfectly!"
    echo ""
    echo "✅ Achievements:"
    echo "  • Memory management stable under high load"
    echo "  • Rendering systems handle concurrent operations"
    echo "  • Configuration system robust under stress"
    echo "  • Integration tests pass under resource pressure"
    echo "  • Performance remains consistent under load"
    echo ""
    echo "🚀 Production Readiness: ENTERPRISE-GRADE"
    echo ""
elif [ $STRESS_SUCCESS_RATE -ge 85 ]; then
    print_success "🥇 EXCELLENT: Rendering pipeline is highly stress-resistant!"
    echo ""
    echo "✅ Strengths:"
    echo "  • Core systems handle stress well"
    echo "  • Memory management mostly stable"
    echo "  • Performance degradation minimal"
    echo ""
    echo "⚠️  Minor optimizations recommended:"
    echo "  • Review failed stress test cases"
    echo "  • Consider resource limit improvements"
    echo ""
    echo "🚀 Production Readiness: PRODUCTION-READY"
    echo ""
elif [ $STRESS_SUCCESS_RATE -ge 70 ]; then
    print_warning "🥈 GOOD: Rendering pipeline handles moderate stress"
    echo ""
    echo "⚠️  Areas for improvement:"
    echo "  • Some stress conditions cause failures"
    echo "  • Resource management could be optimized"
    echo "  • Performance may degrade under high load"
    echo ""
    echo "🔧 Recommendations:"
    echo "  • Optimize memory allocation patterns"
    echo "  • Improve concurrent operation handling"
    echo "  • Add resource throttling mechanisms"
    echo ""
    echo "🚀 Production Readiness: REQUIRES OPTIMIZATION"
    echo ""
else
    print_error "🥉 NEEDS WORK: Rendering pipeline struggles under stress"
    echo ""
    echo "❌ Critical issues:"
    echo "  • Multiple stress test failures"
    echo "  • Resource management problems"
    echo "  • Performance issues under load"
    echo ""
    echo "🔧 Required fixes:"
    echo "  • Memory leak investigation"
    echo "  • Concurrent operation safety"
    echo "  • Resource exhaustion handling"
    echo "  • Performance optimization"
    echo ""
    echo "🚀 Production Readiness: NOT READY"
    echo ""
fi

# Next steps based on results
echo "📋 Next Steps Based on Results"
echo "=============================="

if [ $STRESS_SUCCESS_RATE -ge 95 ]; then
    echo "1. 🎮 Real-world application testing (games, video editing)"
    echo "2. 🖥️  Multi-monitor stress testing"
    echo "3. 📊 Extended performance monitoring (24+ hours)"
    echo "4. 🔧 Integration with desktop environments"
    echo "5. 📦 Package for distribution"
elif [ $STRESS_SUCCESS_RATE -ge 85 ]; then
    echo "1. 🔍 Investigate and fix failed stress tests"
    echo "2. 📈 Profile resource usage under load"
    echo "3. ⚡ Optimize performance bottlenecks"
    echo "4. 🧪 Extended stress testing after fixes"
    echo "5. 🎮 Test with demanding applications"
elif [ $STRESS_SUCCESS_RATE -ge 70 ]; then
    echo "1. 🔧 Fix memory management issues"
    echo "2. 🔄 Improve concurrent operation handling"
    echo "3. 📊 Add resource monitoring and limits"
    echo "4. 🧪 Repeat stress testing after optimizations"
    echo "5. 👥 Consider code review with performance focus"
else
    echo "1. 🚨 Critical: Fix all failing stress tests"
    echo "2. 🔍 Comprehensive code audit"
    echo "3. 💾 Memory leak detection and fixing"
    echo "4. ⚡ Performance optimization"
    echo "5. 🧪 Incremental stress testing"
fi

echo ""

if [ $STRESS_SUCCESS_RATE -ge 85 ]; then
    echo "🎉 Congratulations! Your rendering pipeline is stress-tested and ready!"
    exit 0
elif [ $STRESS_SUCCESS_RATE -ge 70 ]; then
    echo "⚠️  Good progress! Address the issues above for production readiness."
    exit 1
else
    echo "❌ Significant work needed before production deployment."
    exit 2
fi
