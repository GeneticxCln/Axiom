#!/bin/bash
# Create separate user session for Axiom testing

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
║                🧪 AXIOM USER SESSION TESTING 🧪               ║
║                                                               ║
║  Create a separate user account for completely isolated       ║
║  Axiom testing. This is the safest method possible.          ║
║                                                               ║
║  The test user will be automatically cleaned up after.       ║
╚═══════════════════════════════════════════════════════════════╝
"
}

create_test_user() {
    local username="axiom-test-$$"
    local homedir="/home/$username"
    
    log_info "Creating test user: $username"
    
    # Create user with temporary home directory
    sudo useradd -m -s /bin/bash "$username"
    
    # Set up basic environment
    sudo mkdir -p "$homedir/.config/axiom"
    
    # Copy Axiom binary and configs
    sudo cp build/axiom "$homedir/"
    sudo chmod +x "$homedir/axiom"
    
    if [[ -f "examples/axiom.conf" ]]; then
        sudo cp examples/axiom.conf "$homedir/.config/axiom/"
    fi
    
    if [[ -f "examples/rules.conf" ]]; then
        sudo cp examples/rules.conf "$homedir/.config/axiom/"
    fi
    
    # Set ownership
    sudo chown -R "$username:$username" "$homedir"
    
    log_success "Test user created: $username"
    echo "$username"
}

cleanup_test_user() {
    local username="$1"
    
    if [[ -n "$username" ]]; then
        log_info "Cleaning up test user: $username"
        
        # Kill any processes belonging to the test user
        sudo pkill -u "$username" || true
        sleep 2
        
        # Remove user and home directory
        sudo userdel -r "$username" 2>/dev/null || {
            log_warning "Manual cleanup may be needed for user: $username"
        }
        
        log_success "Test user cleaned up"
    fi
}

run_user_session_test() {
    local test_user
    test_user=$(create_test_user)
    
    # Trap cleanup on exit
    trap "cleanup_test_user '$test_user'" EXIT
    
    log_info "Setting up test session for user: $test_user"
    
    echo "
🎮 AXIOM SEPARATE USER SESSION TEST
═══════════════════════════════════════════════════════════════

This will:
1. Switch to TTY2 (Ctrl+Alt+F2)
2. Login as the test user: $test_user
3. Run Axiom as the main compositor
4. Test all features in complete isolation

📋 Instructions:
1. Press Ctrl+Alt+F2 to switch to TTY2
2. Login with username: $test_user (no password needed)
3. Run: ./axiom
4. Test Axiom features
5. Press Super+Q to quit
6. Type 'exit' to logout
7. Press Ctrl+Alt+F1 to return to this session

⚠️  Important:
- This test user has NO password (sudo disabled for security)
- The user will be automatically deleted after testing
- Your main session remains completely untouched

Preparing test environment...
"
    
    # Create startup script for the test user
    sudo tee "/home/$test_user/start-axiom.sh" > /dev/null << 'EOF'
#!/bin/bash
echo "
╔══════════════════════════════════════════════════════════════╗
║                    🚀 AXIOM TEST SESSION 🚀                  ║
║                                                              ║
║  You are now logged in as a temporary test user.            ║
║  Your main system is completely safe and unaffected.        ║
║                                                              ║
║  Commands to try:                                            ║
║    ./axiom          - Start Axiom compositor                 ║
║    exit             - Exit this test session                ║
║                                                              ║
║  Axiom shortcuts (once started):                            ║
║    Super+Return     - Launch terminal                       ║
║    Super+Q          - Quit Axiom                            ║
║    Super+T          - Toggle tiling                         ║
║    Super+1-9        - Switch workspaces                     ║
╚══════════════════════════════════════════════════════════════╝

Starting test environment...
"

# Set up basic environment
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_RUNTIME_DIR="/tmp/axiom-test-runtime-$$"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

echo "Test user: $(whoami)"
echo "Home directory: $HOME"
echo "Available commands:"
echo "  ./axiom          - Start Axiom compositor"
echo "  exit             - Exit test session"
echo ""
echo "Ready for testing! Run './axiom' to start."
EOF
    
    sudo chmod +x "/home/$test_user/start-axiom.sh"
    sudo chown "$test_user:$test_user" "/home/$test_user/start-axiom.sh"
    
    # Set up auto-start script in profile
    sudo tee "/home/$test_user/.bash_profile" > /dev/null << EOF
# Auto-start Axiom test environment
if [[ -f ~/start-axiom.sh ]]; then
    ~/start-axiom.sh
fi
EOF
    
    sudo chown "$test_user:$test_user" "/home/$test_user/.bash_profile"
    
    log_success "Test environment ready!"
    
    echo "
🎯 Next Steps:
1. Press Ctrl+Alt+F2 to switch to TTY2
2. Login as: $test_user (no password)
3. Run: ./axiom
4. Test all features
5. Press Super+Q to quit Axiom
6. Type 'exit' to logout
7. Press Ctrl+Alt+F1 to return here

The test user will be automatically cleaned up when you return.
Press Enter when you're ready to continue or Ctrl+C to cancel.
"
    
    read -p "Press Enter to continue..."
    
    log_info "Test user session is ready. Switch to TTY2 when ready."
    
    # Wait for user to finish testing
    echo "Waiting for you to finish testing..."
    echo "Press Enter here when you're done testing and back on TTY1."
    read -p "Press Enter when testing is complete..."
}

show_options() {
    echo "
🎯 Select testing approach:

1) 🪟  Nested Window Test (safest, easiest)
   - Runs Axiom in a window within your current session
   - No session switching needed
   - Completely isolated

2) 👤  Separate User Session Test (most isolated)
   - Creates temporary user account
   - Full compositor testing on TTY2
   - Complete system isolation

3) ❌  Cancel

"
    read -p "Enter choice (1-3): " choice
    
    case $choice in
        1)
            log_info "Starting nested window test..."
            exec ./test-axiom-safe.sh
            ;;
        2)
            run_user_session_test
            ;;
        3)
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
    # Check if running as root
    if [[ $EUID -eq 0 ]]; then
        log_error "Don't run this script as root!"
        exit 1
    fi
    
    print_banner
    show_options
    
    log_success "🎉 Testing completed safely!"
}

main "$@"
