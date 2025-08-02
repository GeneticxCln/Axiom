#!/bin/bash

# Axiom Auto-Updater - Intelligent dependency monitoring and automatic updates
# Monitors wlroots, system packages, and Axiom repository for updates
# Automatically rebuilds and deploys when dependencies change

set -e

# Configuration
AXIOM_DIR="/home/alex/Projects /Axiom"
LOG_FILE="/var/log/axiom-auto-updater.log"
LOCK_FILE="/tmp/axiom-updater.lock"
CONFIG_FILE="$HOME/.config/axiom-updater.conf"
BACKUP_DIR="/var/backups/axiom"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
DEFAULT_CHECK_INTERVAL=3600  # 1 hour
DEFAULT_AUTO_UPDATE=true
DEFAULT_BACKUP_BEFORE_UPDATE=true
DEFAULT_RUN_TESTS=true
DEFAULT_NOTIFY_USER=true

# Logging function
log() {
    local level=$1
    shift
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "${timestamp} [${level}] ${message}" | sudo tee -a "$LOG_FILE" >/dev/null
    echo -e "${message}"
}

# Load configuration
load_config() {
    if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
    else
        # Create default config
        mkdir -p "$(dirname "$CONFIG_FILE")"
        cat > "$CONFIG_FILE" << EOF
# Axiom Auto-Updater Configuration
CHECK_INTERVAL=${DEFAULT_CHECK_INTERVAL}
AUTO_UPDATE=${DEFAULT_AUTO_UPDATE}
BACKUP_BEFORE_UPDATE=${DEFAULT_BACKUP_BEFORE_UPDATE}
RUN_TESTS=${DEFAULT_RUN_TESTS}
NOTIFY_USER=${DEFAULT_NOTIFY_USER}
AXIOM_REPO_URL="https://github.com/GeneticxCln/Axiom.git"
WLROOTS_MIN_VERSION="0.19"
EOF
        source "$CONFIG_FILE"
        log "INFO" "📝 Created default configuration at $CONFIG_FILE"
    fi
}

# Check if script is already running
check_lock() {
    if [ -f "$LOCK_FILE" ]; then
        local pid=$(cat "$LOCK_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            log "WARN" "⚠️  Auto-updater is already running (PID: $pid)"
            exit 1
        else
            rm -f "$LOCK_FILE"
        fi
    fi
    echo $$ > "$LOCK_FILE"
}

# Cleanup function
cleanup() {
    rm -f "$LOCK_FILE"
    log "INFO" "🧹 Cleanup completed"
}

trap cleanup EXIT

# Create necessary directories
setup_environment() {
    sudo mkdir -p "$(dirname "$LOG_FILE")" "$BACKUP_DIR"
    sudo touch "$LOG_FILE"
    sudo chown "$USER:$USER" "$LOG_FILE"
}

# Get current wlroots version
get_wlroots_version() {
    if command -v pkg-config >/dev/null 2>&1; then
        pkg-config --modversion wlroots 2>/dev/null || echo "not-found"
    else
        echo "pkg-config-missing"
    fi
}

# Get current wlroots package version from system
get_system_wlroots_version() {
    if command -v pacman >/dev/null 2>&1; then
        pacman -Q wlroots 2>/dev/null | awk '{print $2}' || echo "not-installed"
    elif command -v dpkg >/dev/null 2>&1; then
        dpkg -l | grep wlroots | awk '{print $3}' || echo "not-installed"
    elif command -v rpm >/dev/null 2>&1; then
        rpm -q wlroots --queryformat '%{VERSION}' 2>/dev/null || echo "not-installed"
    else
        echo "unknown-package-manager"
    fi
}

# Check for available wlroots updates
check_wlroots_updates() {
    log "INFO" "🔍 Checking for wlroots updates..."
    
    local current_version=$(get_system_wlroots_version)
    log "INFO" "📦 Current wlroots version: $current_version"
    
    if command -v pacman >/dev/null 2>&1; then
        # Arch/CachyOS
        local available_version=$(pacman -Si wlroots 2>/dev/null | grep Version | awk '{print $3}' || echo "unknown")
        log "INFO" "📦 Available wlroots version: $available_version"
        
        if [ "$current_version" != "$available_version" ] && [ "$available_version" != "unknown" ]; then
            log "INFO" "🆙 wlroots update available: $current_version -> $available_version"
            return 0
        fi
    elif command -v apt >/dev/null 2>&1; then
        # Debian/Ubuntu
        apt list --upgradable 2>/dev/null | grep -q wlroots && return 0
    fi
    
    log "INFO" "✅ wlroots is up to date"
    return 1
}

# Check for Axiom repository updates
check_axiom_updates() {
    log "INFO" "🔍 Checking for Axiom repository updates..."
    
    cd "$AXIOM_DIR"
    git fetch origin >/dev/null 2>&1
    
    local local_commit=$(git rev-parse HEAD)
    local remote_commit=$(git rev-parse origin/main)
    
    if [ "$local_commit" != "$remote_commit" ]; then
        log "INFO" "🆙 Axiom updates available"
        log "INFO" "   Local:  $local_commit"
        log "INFO" "   Remote: $remote_commit"
        return 0
    fi
    
    log "INFO" "✅ Axiom is up to date"
    return 1
}

# Backup current installation
backup_current_installation() {
    if [ "$BACKUP_BEFORE_UPDATE" = "true" ]; then
        log "INFO" "💾 Creating backup..."
        local backup_timestamp=$(date '+%Y%m%d_%H%M%S')
        local backup_path="$BACKUP_DIR/axiom_$backup_timestamp"
        
        sudo mkdir -p "$backup_path"
        
        # Backup binary if it exists
        if [ -f "/usr/local/bin/axiom" ]; then
            sudo cp "/usr/local/bin/axiom" "$backup_path/"
        fi
        
        # Backup desktop files
        if [ -f "/usr/share/wayland-sessions/axiom.desktop" ]; then
            sudo cp "/usr/share/wayland-sessions/axiom.desktop" "$backup_path/"
        fi
        
        if [ -f "/usr/share/wayland-sessions/axiom.session" ]; then
            sudo cp "/usr/share/wayland-sessions/axiom.session" "$backup_path/"
        fi
        
        # Backup source state
        cd "$AXIOM_DIR"
        git rev-parse HEAD > "$backup_path/git_commit.txt"
        
        log "INFO" "✅ Backup created at $backup_path"
        
        # Keep only last 5 backups
        sudo find "$BACKUP_DIR" -maxdepth 1 -name "axiom_*" -type d | sort -r | tail -n +6 | sudo xargs rm -rf
    fi
}

# Update system dependencies
update_dependencies() {
    log "INFO" "📦 Updating system dependencies..."
    
    if command -v pacman >/dev/null 2>&1; then
        # Arch/CachyOS
        sudo pacman -Syu --noconfirm wlroots wayland meson ninja
    elif command -v apt >/dev/null 2>&1; then
        # Debian/Ubuntu
        sudo apt update && sudo apt upgrade -y
        sudo apt install -y libwlroots-dev libwayland-dev meson ninja-build
    elif command -v dnf >/dev/null 2>&1; then
        # Fedora
        sudo dnf update -y wlroots-devel wayland-devel meson ninja-build
    else
        log "WARN" "⚠️  Unknown package manager, skipping dependency update"
    fi
    
    log "INFO" "✅ Dependencies updated"
}

# Build and test Axiom
build_axiom() {
    log "INFO" "🔨 Building Axiom..."
    
    cd "$AXIOM_DIR"
    
    # Clean previous build
    if [ -d "build" ]; then
        rm -rf build
    fi
    
    # Configure and build
    meson setup build
    ninja -C build
    
    if [ "$RUN_TESTS" = "true" ]; then
        log "INFO" "🧪 Running tests..."
        ninja -C build test
        
        if [ $? -eq 0 ]; then
            log "INFO" "✅ All tests passed"
        else
            log "ERROR" "❌ Tests failed, aborting update"
            return 1
        fi
    fi
    
    log "INFO" "✅ Build completed successfully"
}

# Deploy updated Axiom
deploy_axiom() {
    log "INFO" "🚀 Deploying Axiom..."
    
    cd "$AXIOM_DIR"
    
    # Install binary
    sudo cp build/axiom /usr/local/bin/axiom
    sudo chmod +x /usr/local/bin/axiom
    
    # Install desktop session files
    sudo mkdir -p /usr/share/wayland-sessions
    sudo cp axiom.desktop /usr/share/wayland-sessions/axiom.desktop
    sudo cp axiom.session /usr/share/wayland-sessions/axiom.session
    
    log "INFO" "✅ Axiom deployed successfully"
}

# Send notification to user
notify_user() {
    if [ "$NOTIFY_USER" = "true" ] && command -v notify-send >/dev/null 2>&1; then
        notify-send "Axiom Auto-Updater" "$1" --icon=dialog-information
    fi
}

# Update Axiom repository
update_axiom_repo() {
    log "INFO" "📥 Updating Axiom repository..."
    
    cd "$AXIOM_DIR"
    git pull origin main
    
    log "INFO" "✅ Repository updated"
}

# Main update process
perform_update() {
    local reason="$1"
    
    log "INFO" "🚀 Starting Axiom update process ($reason)"
    
    backup_current_installation
    
    if [[ "$reason" == *"wlroots"* ]]; then
        update_dependencies
    fi
    
    if [[ "$reason" == *"repository"* ]]; then
        update_axiom_repo
    fi
    
    build_axiom || {
        log "ERROR" "❌ Build failed, update aborted"
        notify_user "❌ Axiom update failed - build error"
        return 1
    }
    
    deploy_axiom
    
    # Get version info
    local version_info="Unknown"
    if [ -f "$AXIOM_DIR/CHANGELOG.md" ]; then
        version_info=$(grep -m1 "##.*\[.*\]" "$AXIOM_DIR/CHANGELOG.md" | sed 's/## *//' || echo "Updated")
    fi
    
    log "INFO" "🎉 Axiom update completed successfully! Version: $version_info"
    notify_user "✅ Axiom updated successfully to $version_info"
}

# Check for updates and perform if needed
check_and_update() {
    local needs_update=false
    local update_reason=""
    
    # Check wlroots updates
    if check_wlroots_updates; then
        needs_update=true
        update_reason="wlroots update"
    fi
    
    # Check Axiom repository updates
    if check_axiom_updates; then
        needs_update=true
        if [ -n "$update_reason" ]; then
            update_reason="$update_reason and repository update"
        else
            update_reason="repository update"
        fi
    fi
    
    if [ "$needs_update" = "true" ]; then
        if [ "$AUTO_UPDATE" = "true" ]; then
            perform_update "$update_reason"
        else
            log "INFO" "📢 Updates available ($update_reason) but auto-update is disabled"
            notify_user "📢 Axiom updates available: $update_reason"
        fi
    else
        log "INFO" "✅ No updates needed"
    fi
}

# Daemon mode
run_daemon() {
    log "INFO" "🔄 Starting Axiom auto-updater daemon (checking every ${CHECK_INTERVAL}s)"
    
    while true; do
        check_and_update
        sleep "$CHECK_INTERVAL"
    done
}

# Install as systemd service
install_service() {
    log "INFO" "📦 Installing Axiom auto-updater as systemd service..."
    
    sudo tee /etc/systemd/system/axiom-auto-updater.service > /dev/null << EOF
[Unit]
Description=Axiom Auto-Updater Service
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=$AXIOM_DIR/axiom-auto-updater.sh --daemon
Restart=always
RestartSec=60

[Install]
WantedBy=multi-user.target
EOF

    sudo systemctl daemon-reload
    sudo systemctl enable axiom-auto-updater.service
    sudo systemctl start axiom-auto-updater.service
    
    log "INFO" "✅ Service installed and started"
    log "INFO" "   Status: sudo systemctl status axiom-auto-updater"
    log "INFO" "   Logs:   sudo journalctl -u axiom-auto-updater -f"
}

# Show status
show_status() {
    echo -e "${CYAN}=== Axiom Auto-Updater Status ===${NC}"
    echo -e "${BLUE}Current wlroots version:${NC} $(get_wlroots_version)"
    echo -e "${BLUE}System wlroots version:${NC} $(get_system_wlroots_version)"
    
    cd "$AXIOM_DIR"
    echo -e "${BLUE}Axiom commit:${NC} $(git rev-parse --short HEAD)"
    echo -e "${BLUE}Axiom branch:${NC} $(git branch --show-current)"
    
    if systemctl is-active --quiet axiom-auto-updater.service; then
        echo -e "${GREEN}Service status: Active${NC}"
    else
        echo -e "${RED}Service status: Inactive${NC}"
    fi
    
    echo -e "${BLUE}Configuration:${NC} $CONFIG_FILE"
    echo -e "${BLUE}Log file:${NC} $LOG_FILE"
}

# Show help
show_help() {
    echo -e "${CYAN}Axiom Auto-Updater - Intelligent dependency monitoring and updates${NC}"
    echo ""
    echo -e "${YELLOW}Usage:${NC}"
    echo "  $0 [OPTION]"
    echo ""
    echo -e "${YELLOW}Options:${NC}"
    echo "  --check          Check for updates without applying them"
    echo "  --update         Force update now"
    echo "  --daemon         Run in daemon mode (continuous monitoring)"
    echo "  --install        Install as systemd service"
    echo "  --status         Show current status"
    echo "  --config         Open configuration file"
    echo "  --logs           Show recent logs"
    echo "  --help           Show this help message"
    echo ""
    echo -e "${YELLOW}Configuration file:${NC} $CONFIG_FILE"
    echo -e "${YELLOW}Log file:${NC} $LOG_FILE"
}

# Main script logic
main() {
    setup_environment
    load_config
    check_lock
    
    case "${1:-}" in
        --check)
            log "INFO" "👀 Manual update check requested"
            check_and_update
            ;;
        --update)
            log "INFO" "🔄 Manual update requested"
            perform_update "manual request"
            ;;
        --daemon)
            run_daemon
            ;;
        --install)
            install_service
            ;;
        --status)
            show_status
            ;;
        --config)
            ${EDITOR:-nano} "$CONFIG_FILE"
            ;;
        --logs)
            tail -f "$LOG_FILE"
            ;;
        --help|"")
            show_help
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            show_help
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
