#!/bin/bash

# Axiom Wayland Compositor Uninstallation Script
# Version: 4.3.8

set -e

AXIOM_NAME="Axiom Wayland Compositor"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
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
    echo -e "${BLUE}"
    echo "======================================"
    echo "  Uninstalling $AXIOM_NAME"
    echo "======================================"
    echo -e "${NC}"
}

# Check if running with proper privileges
check_privileges() {
    if [[ $EUID -eq 0 ]]; then
        log_error "This script should NOT be run as root!"
        log_info "It will ask for sudo privileges when needed."
        exit 1
    fi
}

# Uninstall Axiom
uninstall_axiom() {
    log_info "Uninstalling $AXIOM_NAME..."
    
    # Remove installed files
    if [[ -d "/usr/local/share/wayland-sessions" ]]; then
        sudo rm -f "/usr/local/share/wayland-sessions/axiom.desktop"
        log_info "Removed desktop entry"
    fi
    
    if [[ -f "/usr/local/bin/axiom" ]]; then
        sudo rm -f "/usr/local/bin/axiom"
        log_info "Removed axiom executable"
    fi

    if [[ -f "/usr/local/bin/axiom-session" ]]; then
        sudo rm -f "/usr/local/bin/axiom-session"
        log_info "Removed axiom session script"
    fi
    
    if [[ -d "/usr/local/etc/axiom" ]]; then
        sudo rm -rf "/usr/local/etc/axiom"
        log_info "Removed system configuration"
    fi
    
    # Remove man pages
    if [[ -f "/usr/local/share/man/man1/axiom.1" ]]; then
        sudo rm -f "/usr/local/share/man/man1/axiom.1"
        log_info "Removed manual page"
    fi

    # Remove user config
    if [[ -d "$HOME/.config/axiom" ]]; then
        rm -rf "$HOME/.config/axiom"
        log_info "Removed user configuration"
    fi
    
    log_success "$AXIOM_NAME uninstalled successfully!"
}

# Main uninstallation flow
main() {
    print_banner
    check_privileges
    uninstall_axiom
}

# Run main function
main "$@"
