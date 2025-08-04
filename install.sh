#!/bin/bash

# Axiom Wayland Compositor Installation Script
# Version: 4.3.8
# Supports: Arch Linux, Ubuntu/Debian, Fedora, openSUSE, and Alpine Linux

set -e

AXIOM_VERSION="4.3.8"
AXIOM_NAME="Axiom Wayland Compositor"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

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
    echo "  $AXIOM_NAME v$AXIOM_VERSION"
    echo "  Professional Wayland Compositor"
    echo "======================================"
    echo -e "${NC}"
}

# Detect distribution
detect_distro() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        DISTRO=$ID
        DISTRO_VERSION=$VERSION_ID
    elif [[ -f /etc/redhat-release ]]; then
        DISTRO="rhel"
    elif [[ -f /etc/debian_version ]]; then
        DISTRO="debian"
    else
        DISTRO="unknown"
    fi
    
    log_info "Detected distribution: $DISTRO ${DISTRO_VERSION:-unknown}"
}

# Check if running with proper privileges
check_privileges() {
    if [[ $EUID -eq 0 ]]; then
        log_error "This script should NOT be run as root!"
        log_info "It will ask for sudo privileges when needed."
        exit 1
    fi
}

# Install dependencies based on distribution
install_dependencies() {
    log_info "Installing dependencies for $DISTRO..."
    
    case $DISTRO in
        "arch"|"manjaro"|"cachyos")
            sudo pacman -Syu --needed \
                wayland wayland-protocols wlroots \
                libxkbcommon libxkbcommon-x11 \
                libxcb xcb-util-wm \
                mesa meson ninja pkg-config \
                git gcc || {
                log_error "Failed to install Arch dependencies"
                exit 1
            }
            ;;
        "ubuntu"|"debian"|"pop"|"zorin")
            sudo apt update
            sudo apt install -y \
                libwayland-dev wayland-protocols \
                libwlroots-dev wlr-protocols \
                libxkbcommon-dev libxkbcommon-x11-dev \
                libxcb1-dev libxcb-composite0-dev libxcb-icccm4-dev \
                libxcb-render0-dev libxcb-res0-dev libxcb-xfixes0-dev \
                libegl-dev libgles2-mesa-dev \
                meson ninja-build pkg-config \
                git build-essential || {
                log_error "Failed to install Ubuntu/Debian dependencies"
                log_warning "You may need to enable universe repository or use a PPA for wlroots"
                exit 1
            }
            ;;
        "fedora")
            sudo dnf install -y \
                wayland-devel wayland-protocols-devel \
                wlroots-devel \
                libxkbcommon-devel libxkbcommon-x11-devel \
                libxcb-devel xcb-util-wm-devel \
                mesa-libEGL-devel mesa-libGLES-devel \
                meson ninja-build pkgconfig \
                git gcc || {
                log_error "Failed to install Fedora dependencies"
                exit 1
            }
            ;;
        "opensuse-tumbleweed"|"opensuse-leap")
            sudo zypper install -y \
                wayland-devel wayland-protocols-devel \
                wlroots-devel \
                libxkbcommon-devel libxkbcommon-x11-devel \
                libxcb-devel xcb-util-wm-devel \
                Mesa-libEGL-devel Mesa-libGLESv2-devel \
                meson ninja pkgconfig \
                git gcc || {
                log_error "Failed to install openSUSE dependencies"
                exit 1
            }
            ;;
        "alpine")
            sudo apk add --no-cache \
                wayland-dev wayland-protocols \
                wlroots-dev \
                libxkbcommon-dev \
                libxcb-dev xcb-util-wm-dev \
                mesa-dev \
                meson ninja pkgconfig \
                git gcc musl-dev || {
                log_error "Failed to install Alpine dependencies"
                exit 1
            }
            ;;
        *)
            log_error "Unsupported distribution: $DISTRO"
            log_info "Supported distributions: Arch Linux, Ubuntu/Debian, Fedora, openSUSE, Alpine Linux"
            log_info "Please install dependencies manually and run: meson setup builddir && ninja -C builddir"
            exit 1
            ;;
    esac
    
    log_success "Dependencies installed successfully!"
}

# Check if all required dependencies are available
check_dependencies() {
    log_info "Verifying dependencies..."
    
    local missing_deps=()
    
    # Check for essential build tools
    command -v meson >/dev/null 2>&1 || missing_deps+=("meson")
    command -v ninja >/dev/null 2>&1 || missing_deps+=("ninja")
    command -v pkg-config >/dev/null 2>&1 || missing_deps+=("pkg-config")
    command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
    
    # Check for wayland-scanner
    command -v wayland-scanner >/dev/null 2>&1 || missing_deps+=("wayland-scanner")
    
    if [[ ${#missing_deps[@]} -ne 0 ]]; then
        log_error "Missing dependencies: ${missing_deps[*]}"
        return 1
    fi
    
    # Check for pkg-config libraries
    local pkg_deps=("wayland-server" "wayland-protocols" "wlroots-0.19" "xkbcommon" "egl" "glesv2")
    for dep in "${pkg_deps[@]}"; do
        if ! pkg-config --exists "$dep" 2>/dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [[ ${#missing_deps[@]} -ne 0 ]]; then
        log_error "Missing pkg-config dependencies: ${missing_deps[*]}"
        return 1
    fi
    
    log_success "All dependencies verified!"
    return 0
}

# Build Axiom
build_axiom() {
    log_info "Building $AXIOM_NAME v$AXIOM_VERSION..."
    
    cd "$SCRIPT_DIR"
    
    # Clean previous build if exists
    if [[ -d builddir ]]; then
        log_info "Cleaning previous build..."
        rm -rf builddir
    fi
    
    # Setup build
    log_info "Setting up build configuration..."
    meson setup builddir --buildtype=release --prefix=/usr/local || {
        log_error "Meson setup failed"
        exit 1
    }
    
    # Compile
    log_info "Compiling (this may take a few minutes)..."
    meson compile -C builddir || {
        log_error "Compilation failed"
        exit 1
    }
    
    # Run tests
    log_info "Running test suite..."
    meson test -C builddir || {
        log_error "Tests failed"
        exit 1
    }
    
    log_success "Build completed successfully!"
}

# Install Axiom
install_axiom() {
    log_info "Installing $AXIOM_NAME..."
    
    cd "$SCRIPT_DIR"
    
    # Install binaries and files
    sudo meson install -C builddir || {
        log_error "Installation failed"
        exit 1
    }
    
    # Create user config directory
    local config_dir="$HOME/.config/axiom"
    if [[ ! -d "$config_dir" ]]; then
        log_info "Creating user configuration directory..."
        mkdir -p "$config_dir"
        
        # Copy example configuration
        if [[ -f "examples/axiom.conf" ]]; then
            cp "examples/axiom.conf" "$config_dir/"
            log_info "Copied example configuration to $config_dir/"
        fi
        
        if [[ -f "examples/rules.conf" ]]; then
            cp "examples/rules.conf" "$config_dir/"
            log_info "Copied example rules to $config_dir/"
        fi
    fi
    
    log_success "Installation completed!"
}

# Post-installation setup
post_install() {
    log_info "Setting up post-installation configuration..."
    
    # Add user to necessary groups (if not already)
    if command -v usermod >/dev/null 2>&1; then
        sudo usermod -a -G video,input "$USER" 2>/dev/null || true
        log_info "Added user to video and input groups"
    fi
    
    # Create desktop entry if not exists
    local desktop_dir="$HOME/.local/share/applications"
    local desktop_file="$desktop_dir/axiom.desktop"
    
    if [[ ! -f "$desktop_file" ]] && [[ -d "$desktop_dir" ]]; then
        cat > "$desktop_file" << EOF
[Desktop Entry]
Name=Axiom Wayland Compositor
Comment=Professional Wayland Compositor
Exec=axiom
Icon=preferences-desktop
Type=Application
Categories=System;
EOF
        log_info "Created desktop entry"
    fi
    
    log_success "Post-installation setup completed!"
}

# Print final instructions
print_instructions() {
    echo
    log_success "$AXIOM_NAME v$AXIOM_VERSION has been installed successfully!"
    echo
    echo -e "${BLUE}Getting Started:${NC}"
    echo "  1. Log out of your current session"
    echo "  2. At the login manager, select 'Axiom' as your session"
    echo "  3. Log in to start using Axiom"
    echo
    echo -e "${BLUE}Configuration:${NC}"
    echo "  • Configuration files: ~/.config/axiom/"
    echo "  • System configuration: /usr/local/etc/axiom/"
    echo "  • Manual: man axiom"
    echo
    echo -e "${BLUE}For Support:${NC}"
    echo "  • Documentation: README.md"
    echo "  • Issues: Check the project repository"
    echo
    echo -e "${YELLOW}Note: You may need to restart your session manager or reboot${NC}"
    echo -e "${YELLOW}for Axiom to appear in the session selection menu.${NC}"
    echo
}

# Main installation flow
main() {
    print_banner
    
    check_privileges
    detect_distro
    
    # Parse command line arguments
    local skip_deps=false
    local force_install=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-deps)
                skip_deps=true
                shift
                ;;
            --force)
                force_install=true
                shift
                ;;
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  --skip-deps    Skip dependency installation"
                echo "  --force        Force installation even if dependencies are missing"
                echo "  --help, -h     Show this help message"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Install dependencies if not skipped
    if [[ "$skip_deps" != true ]]; then
        install_dependencies
    fi
    
    # Check dependencies
    if ! check_dependencies; then
        if [[ "$force_install" != true ]]; then
            log_error "Dependency check failed. Use --force to continue anyway."
            exit 1
        else
            log_warning "Continuing with missing dependencies (--force specified)"
        fi
    fi
    
    # Build and install
    build_axiom
    install_axiom
    post_install
    print_instructions
}

# Run main function
main "$@"
