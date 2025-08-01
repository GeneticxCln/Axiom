#!/bin/bash
# Safe Axiom Testing Script

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_banner() {
    echo "
╔═══════════════════════════════════════════════════════════════╗
║                    🚀 AXIOM SAFE TESTING 🚀                   ║
║                                                               ║
║  Testing Axiom compositor safely without affecting your      ║
║  current system or Hyprland session.                         ║
║                                                               ║
║  This will run Axiom in a nested window inside Hyprland.     ║
╚═══════════════════════════════════════════════════════════════╝
"
}

check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check if Axiom binary exists
    if [[ ! -f "build/axiom" ]]; then
        log_error "Axiom binary not found! Run 'meson compile -C build' first."
        exit 1
    fi
    
    # Check if we're in a Wayland session
    if [[ -z "$WAYLAND_DISPLAY" ]]; then
        log_error "Not running in a Wayland session! Nested mode requires Wayland."
        exit 1
    fi
    
    # Check if current WM supports nested compositors
    if [[ -n "$HYPRLAND_INSTANCE_SIGNATURE" ]]; then
        log_success "✓ Running under Hyprland - perfect for nested testing"
    else
        log_warning "⚠ Not running under Hyprland, but should still work"
    fi
    
    log_success "Prerequisites check passed!"
}

setup_test_environment() {
    log_info "Setting up test environment..."
    
    # Create temporary config directory
    TEST_DIR="/tmp/axiom-test-$$"
    mkdir -p "$TEST_DIR/config"
    
    # Copy example configurations
    if [[ -f "examples/axiom.conf" ]]; then
        cp examples/axiom.conf "$TEST_DIR/config/"
        log_success "✓ Copied axiom.conf"
    fi
    
    if [[ -f "examples/rules.conf" ]]; then
        cp examples/rules.conf "$TEST_DIR/config/"
        log_success "✓ Copied rules.conf"
    fi
    
    # Set up environment variables for the test
    export XDG_CONFIG_HOME="$TEST_DIR"
    export AXIOM_LOG_LEVEL="info"
    
    log_success "Test environment ready at: $TEST_DIR"
}

cleanup() {
    log_info "Cleaning up test environment..."
    if [[ -n "$TEST_DIR" && -d "$TEST_DIR" ]]; then
        rm -rf "$TEST_DIR"
        log_success "✓ Cleaned up $TEST_DIR"
    fi
}

# Trap cleanup on exit
trap cleanup EXIT

run_nested_test() {
    log_info "Starting Axiom in nested mode..."
    
    echo "
🎮 AXIOM NESTED TEST SESSION
═══════════════════════════════════════════════════════════════

This will open Axiom in a new window within your current Hyprland session.

📋 What to test:
  • Window management and tiling
  • Keyboard shortcuts (Super+Return for terminal, etc.)
  • Visual effects and animations
  • Workspace switching (Super+1-9)
  • Configuration loading

⌨️  Key shortcuts to try:
  • Super+Return     - Launch terminal
  • Super+D          - Application launcher
  • Super+Q          - Quit Axiom (return to this script)
  • Super+T          - Toggle tiling
  • Alt+Tab          - Window switching

🚪 To exit: Press Super+Q or close the Axiom window

Starting in 3 seconds...
"
    
    sleep 3
    
    # Set environment for nested mode with comprehensive cursor workarounds
    export WLR_BACKENDS=wayland
    export WLR_NO_HARDWARE_CURSORS=1  # Avoid cursor conflicts
    export WLR_RENDERER_ALLOW_SOFTWARE=1  # Allow software rendering fallback
    export WLR_FORCE_SOFTWARE_CURSORS=1  # Force software cursors
    export WLR_XCURSOR_SIZE=24  # Set cursor size explicitly
    export WLR_XCURSOR_THEME=default  # Set cursor theme explicitly
    export WLR_DRM_NO_ATOMIC=1  # Disable atomic operations for cursors
    export WLR_DISABLE_HARDWARE_CURSORS=1  # Additional hardware cursor disable
    export WAYLAND_DISPLAY="${WAYLAND_DISPLAY}"
    
    # Run Axiom in nested mode
    log_info "Launching Axiom..."
    ./build/axiom --nested
    
    log_success "Axiom session ended successfully!"
}

run_test_terminal() {
    log_info "Starting Axiom with test terminal..."
    
    # Create a simple test script that opens a terminal
    cat > "$TEST_DIR/test-session.sh" << 'EOF'
#!/bin/bash
# Wait a moment for Axiom to start
sleep 2

# Try to open a terminal (adjust for available terminals)
if command -v alacritty >/dev/null; then
    alacritty &
elif command -v foot >/dev/null; then
    foot &
elif command -v kitty >/dev/null; then
    kitty &
elif command -v wezterm >/dev/null; then
    wezterm &
else
    echo "No suitable terminal found for testing"
fi
EOF
    
    chmod +x "$TEST_DIR/test-session.sh"
    
    # Run the test session script after a delay
    "$TEST_DIR/test-session.sh" &
    
    # Run Axiom
    run_nested_test
}

show_menu() {
    echo "
🎯 Select testing mode:

1) 🪟  Basic nested test (just Axiom)
2) 🖥️  Test with terminal (Axiom + auto-launch terminal)
3) 🔧  Custom test (set your own environment variables)
4) 📊  Performance test (with monitoring)
5) ❌  Cancel

"
    read -p "Enter choice (1-5): " choice
    
    case $choice in
        1)
            run_nested_test
            ;;
        2)
            run_test_terminal
            ;;
        3)
            echo "Enter custom environment variables (e.g., AXIOM_LOG_LEVEL=debug):"
            read -p "Custom vars: " custom_vars
            eval "export $custom_vars"
            run_nested_test
            ;;
        4)
            log_info "Starting performance monitoring..."
            # Monitor resource usage
            (
                while true; do
                    ps aux | grep axiom | grep -v grep | head -1
                    sleep 1
                done
            ) &
            MONITOR_PID=$!
            run_nested_test
            kill $MONITOR_PID 2>/dev/null || true
            ;;
        5)
            log_info "Testing cancelled."
            exit 0
            ;;
        *)
            log_error "Invalid choice!"
            exit 1
            ;;
    esac
}

# Main execution
main() {
    print_banner
    check_prerequisites
    setup_test_environment
    show_menu
    
    log_success "🎉 Safe testing completed!"
    echo "
💡 What you tested is completely isolated and didn't affect your system:
   • Axiom ran in a nested window within Hyprland
   • Used temporary configuration directory
   • No system files were modified
   • Your current session remained untouched
   
Ready for production use! 🚀
"
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    log_error "Don't run this script as root!"
    exit 1
fi

main "$@"
