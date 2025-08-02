#!/bin/bash

# Axiom Wayland Compositor - Comprehensive Debugger Script
# This script analyzes the Axiom project for common issues and provides debugging information

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Project paths
PROJECT_DIR="/home/alex/Projects /Axiom"
BUILD_DIR="$PROJECT_DIR/build"
SRC_DIR="$PROJECT_DIR/src"
INCLUDE_DIR="$PROJECT_DIR/include"

# Counters for issues
CRITICAL_ISSUES=0
WARNINGS=0
SUGGESTIONS=0

print_header() {
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║                    AXIOM WAYLAND COMPOSITOR DEBUGGER                         ║${NC}"
    echo -e "${BLUE}║                         Comprehensive Analysis Tool                          ║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

print_section() {
    echo -e "${CYAN}━━━ $1 ━━━${NC}"
}

print_critical() {
    echo -e "${RED}🚨 CRITICAL: $1${NC}"
    ((CRITICAL_ISSUES++))
}

print_warning() {
    echo -e "${YELLOW}⚠️  WARNING: $1${NC}"
    ((WARNINGS++))
}

print_suggestion() {
    echo -e "${BLUE}💡 SUGGESTION: $1${NC}"
    ((SUGGESTIONS++))
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_info() {
    echo -e "${PURPLE}ℹ️  INFO: $1${NC}"
}

check_environment() {
    print_section "ENVIRONMENT CHECK"
    
    # Check if we're in the right directory
    if [[ ! -d "$PROJECT_DIR" ]]; then
        print_critical "Project directory not found: $PROJECT_DIR"
        return 1
    fi
    
    cd "$PROJECT_DIR" || exit 1
    print_success "Project directory found and accessible"
    
    # Check for space in directory name
    if [[ "$PROJECT_DIR" == *" "* ]]; then
        print_warning "Directory name contains spaces, this can cause issues with some tools"
        print_suggestion "Consider renaming to: mv '$PROJECT_DIR' '${PROJECT_DIR// /}'"
    fi
    
    # Check operating system
    if [[ $(uname -s) != "Linux" ]]; then
        print_critical "Axiom only supports Linux systems"
        return 1
    fi
    print_success "Running on Linux system"
    
    # Check user groups
    if ! groups | grep -q video; then
        print_warning "User not in 'video' group - may have DRM access issues"
        print_suggestion "Add user to video group: sudo usermod -a -G video $USER"
    else
        print_success "User in 'video' group"
    fi
    
    echo ""
}

check_dependencies() {
    print_section "DEPENDENCY CHECK"
    
    local missing_deps=()
    local deps=(
        "wayland-scanner"
        "meson"
        "ninja"
        "gcc"
        "pkg-config"
    )
    
    for dep in "${deps[@]}"; do
        if command -v "$dep" >/dev/null 2>&1; then
            print_success "$dep found ($(command -v $dep))"
        else
            print_critical "$dep not found"
            missing_deps+=("$dep")
        fi
    done
    
    # Check pkg-config packages
    local pkg_deps=(
        "wayland-server"
        "wayland-client" 
        "wayland-protocols"
        "wlroots-0.19"
        "xkbcommon"
        "egl"
        "glesv2"
    )
    
    for pkg in "${pkg_deps[@]}"; do
        if pkg-config --exists "$pkg" 2>/dev/null; then
            local version=$(pkg-config --modversion "$pkg" 2>/dev/null || echo "unknown")
            print_success "$pkg found (version: $version)"
        else
            print_critical "pkg-config package '$pkg' not found"
            missing_deps+=("$pkg")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        print_critical "Missing dependencies detected"
        echo "Install missing dependencies on CachyOS/Arch:"
        echo "sudo pacman -S wayland wayland-protocols wlroots0.19 pixman libdrm meson ninja gcc pkg-config"
    fi
    
    echo ""
}

check_build_system() {
    print_section "BUILD SYSTEM CHECK"
    
    # Check for meson.build
    if [[ ! -f "meson.build" ]]; then
        print_critical "meson.build not found"
        return 1
    fi
    print_success "meson.build found"
    
    # Check build directory
    if [[ ! -d "$BUILD_DIR" ]]; then
        print_info "Build directory not found, attempting to create..."
        if meson setup build 2>/dev/null; then
            print_success "Build directory created successfully"
        else
            print_critical "Failed to create build directory"
            return 1
        fi
    else
        print_success "Build directory exists"
    fi
    
    # Check if build is configured
    if [[ ! -f "$BUILD_DIR/build.ninja" ]]; then
        print_warning "Build not configured, reconfiguring..."
        if meson setup build --reconfigure 2>/dev/null; then
            print_success "Build reconfigured successfully"
        else
            print_critical "Failed to configure build"
            return 1
        fi
    fi
    
    echo ""
}

check_compilation() {
    print_section "COMPILATION CHECK"
    
    cd "$PROJECT_DIR" || exit 1
    
    print_info "Attempting to compile project..."
    if meson compile -C build 2>&1 | tee /tmp/axiom_compile.log; then
        print_success "Project compiled successfully"
        
        # Check for warnings
        if grep -i warning /tmp/axiom_compile.log >/dev/null; then
            print_warning "Compilation warnings detected"
            echo "Warnings found:"
            grep -i warning /tmp/axiom_compile.log | head -5
        fi
    else
        print_critical "Compilation failed"
        echo "Compilation errors:"
        tail -20 /tmp/axiom_compile.log
        return 1
    fi
    
    # Check if executable exists
    if [[ -f "$BUILD_DIR/axiom" ]]; then
        print_success "Axiom executable created"
        print_info "Executable size: $(du -h "$BUILD_DIR/axiom" | cut -f1)"
    else
        print_critical "Axiom executable not found after compilation"
    fi
    
    echo ""
}

check_tests() {
    print_section "TEST SUITE CHECK"
    
    cd "$PROJECT_DIR" || exit 1
    
    print_info "Running test suite..."
    if meson test -C build 2>&1 | tee /tmp/axiom_tests.log; then
        # Extract test results more reliably
        local passed=0
        local failed=0
        
        if grep -q "Ok:" /tmp/axiom_tests.log; then
            passed=$(grep "Ok:" /tmp/axiom_tests.log | awk '{print $2}' | head -1)
        fi
        
        if grep -q "Fail:" /tmp/axiom_tests.log; then
            failed=$(grep "Fail:" /tmp/axiom_tests.log | awk '{print $2}' | head -1)
        fi
        
        # Ensure numeric values
        passed=${passed:-0}
        failed=${failed:-0}
        
        if [[ $failed -eq 0 ]]; then
            print_success "All tests passed ($passed tests)"
        else
            print_critical "$failed tests failed, $passed tests passed"
            echo "Failed tests:"
            grep "FAIL" /tmp/axiom_tests.log
        fi
    else
        print_warning "Test suite execution had issues"
    fi
    
    echo ""
}

check_code_quality() {
    print_section "CODE QUALITY CHECK"
    
    cd "$PROJECT_DIR" || exit 1
    
    # Check for TODO/FIXME/HACK comments
    local todo_count=$(find src include -name "*.c" -o -name "*.h" | xargs grep -c "TODO\|FIXME\|HACK\|BUG" 2>/dev/null | awk -F: '{sum += $2} END {print sum+0}')
    if [[ $todo_count -gt 0 ]]; then
        print_info "Found $todo_count TODO/FIXME/HACK/BUG comments"
        echo "Most common issues:"
        find src include -name "*.c" -o -name "*.h" | xargs grep -n "TODO\|FIXME\|HACK\|BUG" 2>/dev/null | head -5
    fi
    
    # Check for potential memory leaks
    local malloc_count=$(find src -name "*.c" | xargs grep -c "malloc\|calloc\|strdup" 2>/dev/null | awk -F: '{sum += $2} END {print sum+0}')
    local free_count=$(find src -name "*.c" | xargs grep -c "free" 2>/dev/null | awk -F: '{sum += $2} END {print sum+0}')
    
    if [[ $malloc_count -gt $free_count ]]; then
        print_warning "Potential memory leak: $malloc_count allocations vs $free_count frees"
        print_suggestion "Review memory management in source files"
    else
        print_success "Memory allocation/deallocation appears balanced"
    fi
    
    # Check for long functions
    print_info "Checking for overly long functions..."
    find src -name "*.c" -exec awk '/^[a-zA-Z_][a-zA-Z0-9_]*.*\(.*\).*\{/ {func=FILENAME":"NR":"$0; lines=1} /^\}/ && func {if(lines>100) print func " - " lines " lines"; func=""; lines=0} func {lines++}' {} \; | head -3
    
    echo ""
}

check_runtime_environment() {
    print_section "RUNTIME ENVIRONMENT CHECK"
    
    # Check for existing compositors
    if pgrep -x "Hyprland\|sway\|gnome-shell\|kwin_wayland" >/dev/null; then
        local compositor=$(pgrep -x "Hyprland\|sway\|gnome-shell\|kwin_wayland" | head -1 | xargs ps -p | tail -1 | awk '{print $4}')
        print_warning "Another compositor is running: $compositor"
        print_suggestion "For testing, use --nested flag or terminate existing compositor"
    else
        print_success "No conflicting compositors detected"
    fi
    
    # Check environment variables
    if [[ -n "$WAYLAND_DISPLAY" ]]; then
        print_info "Running in Wayland session (WAYLAND_DISPLAY=$WAYLAND_DISPLAY)"
    fi
    
    if [[ -n "$DISPLAY" ]]; then
        print_info "X11 display detected (DISPLAY=$DISPLAY)"
    fi
    
    # Check session type
    if [[ "$XDG_SESSION_TYPE" == "wayland" ]]; then
        print_success "Running in Wayland session"
    elif [[ "$XDG_SESSION_TYPE" == "x11" ]]; then
        print_info "Running in X11 session"
    fi
    
    echo ""
}

check_protocol_files() {
    print_section "PROTOCOL FILES CHECK"
    
    cd "$PROJECT_DIR" || exit 1
    
    # Check for generated protocol files
    if [[ -f "xdg-shell-protocol.h" ]]; then
        print_success "XDG shell protocol header found"
    else
        print_warning "XDG shell protocol header missing"
        print_info "Attempting to generate..."
        if wayland-scanner server-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.h 2>/dev/null; then
            print_success "Protocol header generated successfully"
        else
            print_critical "Failed to generate protocol header"
        fi
    fi
    
    echo ""
}

run_basic_functionality_test() {
    print_section "BASIC FUNCTIONALITY TEST"
    
    cd "$PROJECT_DIR" || exit 1
    
    if [[ ! -f "$BUILD_DIR/axiom" ]]; then
        print_critical "Axiom executable not found, cannot test"
        return 1
    fi
    
    # Test help output
    print_info "Testing help output..."
    if timeout 5 "$BUILD_DIR/axiom" --help >/dev/null 2>&1; then
        print_success "Help command works"
    else
        print_warning "Help command failed or timed out"
    fi
    
    # Test nested mode startup (brief test)
    if [[ -n "$WAYLAND_DISPLAY" ]]; then
        print_info "Testing nested mode startup..."
        if timeout 3 "$BUILD_DIR/axiom" --nested >/dev/null 2>&1; then
            print_success "Nested mode startup works"
        else
            local exit_code=$?
            if [[ $exit_code -eq 124 ]]; then
                print_success "Nested mode started (timed out as expected)"
            else
                print_warning "Nested mode startup failed (exit code: $exit_code)"
            fi
        fi
    else
        print_info "Skipping nested test (no Wayland display available)"
    fi
    
    echo ""
}

check_configuration_files() {
    print_section "CONFIGURATION FILES CHECK"
    
    cd "$PROJECT_DIR" || exit 1
    
    # Check main config file
    if [[ -f "axiom.conf" ]]; then
        print_success "Main configuration file found"
    elif [[ -f "examples/axiom.conf" ]]; then
        print_success "Example configuration file found"
    else
        print_warning "No configuration file found"
        print_suggestion "Copy examples/axiom.conf to axiom.conf for testing"
    fi
    
    # Check rules file
    if [[ -f "rules.conf" ]]; then
        print_success "Window rules file found"
    elif [[ -f "examples/rules.conf" ]]; then
        print_success "Example rules file found"
    else
        print_warning "No window rules file found"
    fi
    
    echo ""
}

analyze_performance() {
    print_section "PERFORMANCE ANALYSIS"
    
    cd "$PROJECT_DIR" || exit 1
    
    # Check executable size
    if [[ -f "$BUILD_DIR/axiom" ]]; then
        local size=$(stat -c%s "$BUILD_DIR/axiom")
        local size_mb=$((size / 1024 / 1024))
        
        if [[ $size_mb -lt 5 ]]; then
            print_success "Executable size: ${size_mb}MB (compact)"
        elif [[ $size_mb -lt 15 ]]; then
            print_success "Executable size: ${size_mb}MB (reasonable)"
        else
            print_warning "Executable size: ${size_mb}MB (large)"
        fi
    fi
    
    # Analyze code statistics
    local c_files=$(find src -name "*.c" | wc -l)
    local h_files=$(find include -name "*.h" | wc -l)
    local total_lines=$(find src include -name "*.c" -o -name "*.h" | xargs wc -l | tail -1 | awk '{print $1}')
    
    print_info "Code statistics:"
    echo "  - C source files: $c_files"
    echo "  - Header files: $h_files" 
    echo "  - Total lines of code: $total_lines"
    
    echo ""
}

generate_report() {
    print_section "ANALYSIS SUMMARY"
    
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║                              ANALYSIS RESULTS                                ║${NC}"
    echo -e "${BLUE}╠═══════════════════════════════════════════════════════════════════════════════╣${NC}"
    
    if [[ $CRITICAL_ISSUES -eq 0 ]]; then
        echo -e "${BLUE}║${GREEN} ✅ CRITICAL ISSUES: $CRITICAL_ISSUES                                                    ${BLUE}║${NC}"
    else
        echo -e "${BLUE}║${RED} 🚨 CRITICAL ISSUES: $CRITICAL_ISSUES                                                    ${BLUE}║${NC}"
    fi
    
    if [[ $WARNINGS -lt 5 ]]; then
        echo -e "${BLUE}║${YELLOW} ⚠️  WARNINGS: $WARNINGS                                                             ${BLUE}║${NC}"
    else
        echo -e "${BLUE}║${RED} ⚠️  WARNINGS: $WARNINGS (many warnings detected)                                    ${BLUE}║${NC}"
    fi
    
    echo -e "${BLUE}║${PURPLE} 💡 SUGGESTIONS: $SUGGESTIONS                                                           ${BLUE}║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════════════════════════╝${NC}"
    
    echo ""
    
    if [[ $CRITICAL_ISSUES -eq 0 && $WARNINGS -lt 3 ]]; then
        echo -e "${GREEN}🎉 PROJECT STATUS: EXCELLENT${NC}"
        echo "Your Axiom compositor appears to be in great shape!"
    elif [[ $CRITICAL_ISSUES -eq 0 && $WARNINGS -lt 8 ]]; then
        echo -e "${YELLOW}👍 PROJECT STATUS: GOOD${NC}"
        echo "Your Axiom compositor is working well with minor issues."
    elif [[ $CRITICAL_ISSUES -lt 3 ]]; then
        echo -e "${YELLOW}⚡ PROJECT STATUS: NEEDS ATTENTION${NC}"
        echo "Address the critical issues and warnings for better stability."
    else
        echo -e "${RED}🔧 PROJECT STATUS: NEEDS MAJOR FIXES${NC}"
        echo "Multiple critical issues detected. Review and fix before use."
    fi
    
    echo ""
    echo -e "${CYAN}Quick fixes you can try:${NC}"
    echo "1. Rebuild project: cd '$PROJECT_DIR' && meson compile -C build"
    echo "2. Run tests: meson test -C build"
    echo "3. Test nested mode: ./build/axiom --nested"
    echo "4. Check logs in /tmp/axiom_*.log for detailed information"
    
    # Save detailed report
    {
        echo "Axiom Debugger Report - $(date)"
        echo "======================================"
        echo "Critical Issues: $CRITICAL_ISSUES"
        echo "Warnings: $WARNINGS"
        echo "Suggestions: $SUGGESTIONS"
        echo ""
        echo "Build Log:"
        [[ -f /tmp/axiom_compile.log ]] && cat /tmp/axiom_compile.log
        echo ""
        echo "Test Log:"
        [[ -f /tmp/axiom_tests.log ]] && cat /tmp/axiom_tests.log
    } > "/tmp/axiom_debug_report_$(date +%Y%m%d_%H%M%S).txt"
    
    echo ""
    echo -e "${BLUE}Detailed report saved to: /tmp/axiom_debug_report_$(date +%Y%m%d_%H%M%S).txt${NC}"
}

# Main execution
main() {
    clear
    print_header
    
    check_environment || exit 1
    check_dependencies
    check_protocol_files
    check_build_system
    check_compilation
    check_tests
    check_configuration_files
    check_code_quality
    check_runtime_environment
    analyze_performance
    run_basic_functionality_test
    
    generate_report
}

# Check if script is being run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
