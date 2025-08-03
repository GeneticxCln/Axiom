#!/bin/bash

# Axiom Updater - Smart Project and wlroots Update Manager
# Automatically updates Axiom project while ensuring wlroots compatibility

set -euo pipefail

# Configuration
AXIOM_DIR="/home/alex/Projects/Axiom"
LOG_FILE="$HOME/.axiom-updater.log"
BACKUP_DIR="$HOME/.axiom-backups"
FORCE_UPDATE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

error() {
    log "${RED}ERROR: $1${NC}"
    exit 1
}

success() {
    log "${GREEN}SUCCESS: $1${NC}"
}

warning() {
    log "${YELLOW}WARNING: $1${NC}"
}

info() {
    log "${BLUE}INFO: $1${NC}"
}

# Check if running as root
check_permissions() {
    if [[ $EUID -eq 0 ]]; then
        error "Don't run this script as root!"
    fi
}

# Create necessary directories
setup_directories() {
    mkdir -p "$BACKUP_DIR"
    mkdir -p "$(dirname "$LOG_FILE")"
    info "Setup directories created"
}

# Get current wlroots version
get_wlroots_version() {
    local current_version
    current_version=$(pacman -Q | grep wlroots | head -1 | awk '{print $2}' | cut -d'-' -f1)
    echo "$current_version"
}

# Get required wlroots version from Axiom
get_required_wlroots() {
    if [[ -f "$AXIOM_DIR/meson.build" ]]; then
        grep "wlroots-" "$AXIOM_DIR/meson.build" | head -1 | sed -n "s/.*wlroots-\([0-9.]*\).*/\1/p"
    else
        echo "unknown"
    fi
}

# Check wlroots compatibility
check_wlroots_compatibility() {
    local current_version required_version
    current_version=$(get_wlroots_version)
    required_version=$(get_required_wlroots)
    
    info "Current wlroots: $current_version"
    info "Required wlroots: $required_version"
    
    if [[ "$current_version" =~ ^${required_version} ]]; then
        success "wlroots compatibility: ✅ COMPATIBLE"
        return 0
    else
        warning "wlroots compatibility: ⚠️  VERSION MISMATCH"
        return 1
    fi
}

# Create backup
create_backup() {
    local backup_name
    backup_name="axiom-backup-$(date +%Y%m%d-%H%M%S)"
    local backup_path="$BACKUP_DIR/$backup_name"
    
    info "Creating backup: $backup_name"
    cp -r "$AXIOM_DIR" "$backup_path"
    success "Backup created at: $backup_path"
    
    # Keep only last 5 backups
    cd "$BACKUP_DIR"
    ls -1t axiom-backup-* | tail -n +6 | xargs -r rm -rf
    info "Old backups cleaned (kept 5 most recent)"
}

# Check for Axiom updates
check_axiom_updates() {
    cd "$AXIOM_DIR"
    
    # Fetch latest changes
    git fetch origin --quiet
    
    local current_commit ahead_count new_commits
    current_commit=$(git rev-parse HEAD)
    ahead_count=$(git rev-list --count HEAD..origin/main 2>/dev/null || echo "0")
    
    if [[ "$ahead_count" -gt 0 ]]; then
        info "Found $ahead_count new commits available"
        new_commits=$(git log --oneline HEAD..origin/main)
        info "New commits:\n$new_commits"
        return 0
    else
        success "Axiom is up to date (no new commits)"
        return 1
    fi
}

# Update Axiom project
update_axiom() {
    cd "$AXIOM_DIR"
    
    info "Updating Axiom project..."
    
    # Pull updates
    git pull origin main
    
    # Check if wlroots requirements changed
    local new_required_version
    new_required_version=$(get_required_wlroots)
    
    if ! check_wlroots_compatibility; then
        warning "wlroots version mismatch detected after update!"
        warning "Required: $new_required_version, Current: $(get_wlroots_version)"
        
        if [[ "$FORCE_UPDATE" == "false" ]]; then
            error "Update aborted due to wlroots incompatibility. Use --force to override."
        else
            warning "Continuing with --force flag (may cause build issues)"
        fi
    fi
    
    success "Axiom project updated successfully"
}

# Rebuild Axiom
rebuild_axiom() {
    cd "$AXIOM_DIR"
    
    info "Rebuilding Axiom..."
    
    # Clean previous build
    if [[ -d "build" ]]; then
        rm -rf build
        info "Cleaned previous build directory"
    fi
    
    # Setup build
    meson setup build || error "Meson setup failed - check wlroots compatibility"
    
    # Compile
    meson compile -C build || error "Compilation failed"
    
    success "Axiom rebuilt successfully"
}

# Run tests
run_tests() {
    cd "$AXIOM_DIR"
    
    info "Running Axiom tests..."
    
    if meson test -C build; then
        success "All tests passed ✅"
        return 0
    else
        error "Tests failed ❌ - Update aborted"
        return 1
    fi
}

# Check system updates for wlroots
check_system_updates() {
    info "Checking for wlroots system updates..."
    
    local available_updates
    available_updates=$(checkupdates 2>/dev/null | grep wlroots || echo "")
    
    if [[ -n "$available_updates" ]]; then
        warning "wlroots system updates available:"
        warning "$available_updates"
        warning "⚠️  DO NOT update wlroots without checking Axiom compatibility first!"
        return 0
    else
        info "No wlroots system updates available"
        return 1
    fi
}

# Install updated Axiom (optional)
install_axiom() {
    cd "$AXIOM_DIR"
    
    if [[ -f "build/axiom" ]]; then
        info "Installing updated Axiom..."
        sudo meson install -C build
        success "Axiom installed successfully"
    else
        warning "No compiled binary found, skipping installation"
    fi
}

# Main update process
main_update() {
    info "🚀 Starting Axiom Update Process"
    
    # Check initial compatibility
    if ! check_wlroots_compatibility && [[ "$FORCE_UPDATE" == "false" ]]; then
        error "Initial wlroots compatibility check failed. Use --force to override."
    fi
    
    # Check for updates
    if check_axiom_updates || [[ "$FORCE_UPDATE" == "true" ]]; then
        # Create backup before updating
        create_backup
        
        # Update project
        update_axiom
        
        # Rebuild
        rebuild_axiom
        
        # Run tests
        run_tests
        
        # Optional: Install
        read -p "Install updated Axiom system-wide? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            install_axiom
        fi
        
        success "🎉 Axiom update completed successfully!"
    else
        info "No updates needed"
    fi
    
    # Check system updates
    check_system_updates
}

# Show help
show_help() {
    cat << EOF
Axiom Updater - Smart Project and wlroots Update Manager

USAGE:
    $0 [OPTIONS]

OPTIONS:
    --force         Force update even with wlroots incompatibility
    --backup-only   Create backup without updating
    --check-only    Check for updates without applying them
    --rebuild       Force rebuild without updating
    --help          Show this help message

EXAMPLES:
    $0                    # Normal update check and apply
    $0 --check-only       # Just check for updates
    $0 --force            # Force update ignoring wlroots warnings
    $0 --backup-only      # Create backup only

FEATURES:
    ✅ Automatic wlroots compatibility checking
    ✅ Safe backup creation before updates
    ✅ Intelligent update detection
    ✅ Comprehensive testing after updates
    ✅ System-wide installation option
    ✅ Rollback capability with backups

LOGS: $LOG_FILE
BACKUPS: $BACKUP_DIR
EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --force)
            FORCE_UPDATE=true
            shift
            ;;
        --backup-only)
            check_permissions
            setup_directories
            create_backup
            exit 0
            ;;
        --check-only)
            check_permissions
            setup_directories
            cd "$AXIOM_DIR"
            check_wlroots_compatibility
            check_axiom_updates
            check_system_updates
            exit 0
            ;;
        --rebuild)
            check_permissions
            setup_directories
            rebuild_axiom
            run_tests
            exit 0
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            error "Unknown option: $1. Use --help for usage information."
            ;;
    esac
done

# Main execution
main() {
    check_permissions
    setup_directories
    
    info "=== Axiom Updater Started ==="
    info "Timestamp: $(date)"
    info "Axiom Directory: $AXIOM_DIR"
    
    main_update
    
    info "=== Axiom Updater Completed ==="
}

# Run main function
main "$@"
