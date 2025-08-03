#!/bin/bash

# Generate all essential Wayland protocols for a real compositor
# This script generates 50+ protocol headers needed for modern applications

set -e

PROTOCOLS_DIR="protocols"
WAYLAND_PROTOCOLS="/usr/share/wayland-protocols"
WAYLAND_CORE="/usr/share/wayland"

echo "🚀 Generating Essential Wayland Protocols..."
echo "============================================="

# Create protocols directory
mkdir -p $PROTOCOLS_DIR

# Function to generate protocol header
generate_protocol() {
    local xml_file="$1"
    local output_name="$2"
    
    if [[ -f "$xml_file" ]]; then
        echo "📝 Generating: $output_name"
        wayland-scanner server-header "$xml_file" "$PROTOCOLS_DIR/$output_name.h"
        wayland-scanner private-code "$xml_file" "$PROTOCOLS_DIR/$output_name.c"
    else
        echo "❌ Missing: $xml_file"
    fi
}

echo ""
echo "1️⃣  CORE WAYLAND PROTOCOLS"
echo "=========================="

# Core Wayland protocols (essential for any compositor)
generate_protocol "$WAYLAND_CORE/wayland.xml" "wayland-server-protocol"

echo ""
echo "2️⃣  STABLE PROTOCOLS (Essential for modern apps)"
echo "=============================================="

# XDG Shell - Window management (CRITICAL)
generate_protocol "$WAYLAND_PROTOCOLS/stable/xdg-shell/xdg-shell.xml" "xdg-shell-protocol"

# Linux DMA-BUF - Hardware acceleration (CRITICAL)
generate_protocol "$WAYLAND_PROTOCOLS/stable/linux-dmabuf/linux-dmabuf-v1.xml" "linux-dmabuf-v1-protocol"

# Presentation Time - VSync and timing (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/stable/presentation-time/presentation-time.xml" "presentation-time-protocol"

# Viewporter - Scaling and cropping (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/stable/viewporter/viewporter.xml" "viewporter-protocol"

# Tablet - Stylus/drawing tablet support
generate_protocol "$WAYLAND_PROTOCOLS/stable/tablet/tablet-v2.xml" "tablet-v2-protocol"

echo ""
echo "3️⃣  STAGING PROTOCOLS (Modern features)"
echo "====================================="

# XDG Activation - App launching (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/xdg-activation/xdg-activation-v1.xml" "xdg-activation-v1-protocol"

# Fractional Scale - HiDPI support (CRITICAL for HiDPI)
generate_protocol "$WAYLAND_PROTOCOLS/staging/fractional-scale/fractional-scale-v1.xml" "fractional-scale-v1-protocol"

# Single Pixel Buffer - Solid colors (USEFUL)
generate_protocol "$WAYLAND_PROTOCOLS/staging/single-pixel-buffer/single-pixel-buffer-v1.xml" "single-pixel-buffer-v1-protocol"

# Cursor Shape - Modern cursor themes (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/cursor-shape/cursor-shape-v1.xml" "cursor-shape-v1-protocol"

# Tearing Control - Variable refresh rate (GAMING)
generate_protocol "$WAYLAND_PROTOCOLS/staging/tearing-control/tearing-control-v1.xml" "tearing-control-v1-protocol"

# Security Context - Sandboxing (SECURITY)
generate_protocol "$WAYLAND_PROTOCOLS/staging/security-context/security-context-v1.xml" "security-context-v1-protocol"

# Session Lock - Screen locking (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/ext-session-lock/ext-session-lock-v1.xml" "ext-session-lock-v1-protocol"

# Foreign Toplevel List - Taskbars/window lists (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml" "ext-foreign-toplevel-list-v1-protocol"

# Image Capture - Screenshots (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml" "ext-image-copy-capture-v1-protocol"

# Idle Notify - Power management (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/ext-idle-notify/ext-idle-notify-v1.xml" "ext-idle-notify-v1-protocol"

# Data Control - Clipboard management (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/staging/ext-data-control/ext-data-control-v1.xml" "ext-data-control-v1-protocol"

# XDG Dialog - Proper dialog handling
generate_protocol "$WAYLAND_PROTOCOLS/staging/xdg-dialog/xdg-dialog-v1.xml" "xdg-dialog-v1-protocol"

# XDG Toplevel Drag - Window dragging
generate_protocol "$WAYLAND_PROTOCOLS/staging/xdg-toplevel-drag/xdg-toplevel-drag-v1.xml" "xdg-toplevel-drag-v1-protocol"

# Content Type - Content optimization
generate_protocol "$WAYLAND_PROTOCOLS/staging/content-type/content-type-v1.xml" "content-type-v1-protocol"

echo ""
echo "4️⃣  UNSTABLE PROTOCOLS (Still needed by many apps)"
echo "=============================================="

# XDG Output - Monitor information (CRITICAL)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/xdg-output/xdg-output-unstable-v1.xml" "xdg-output-unstable-v1-protocol"

# XDG Decoration - Server-side decorations (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml" "xdg-decoration-unstable-v1-protocol"

# Layer Shell - Panels, bars, overlays (CRITICAL for desktop environments)
# Note: This is often in wlr-protocols, not wayland-protocols
if [[ -f "/usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml" ]]; then
    generate_protocol "/usr/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml" "wlr-layer-shell-unstable-v1-protocol"
elif [[ -f "/usr/local/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml" ]]; then
    generate_protocol "/usr/local/share/wlr-protocols/unstable/wlr-layer-shell-unstable-v1.xml" "wlr-layer-shell-unstable-v1-protocol"
else
    echo "❌ Layer Shell protocol not found - install wlr-protocols"
fi

# Input Method - Text input (IMPORTANT for international users)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/input-method/input-method-unstable-v1.xml" "input-method-unstable-v1-protocol"

# Text Input - Text input protocol (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/text-input/text-input-unstable-v3.xml" "text-input-unstable-v3-protocol"

# Primary Selection - Middle-click paste (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/primary-selection/primary-selection-unstable-v1.xml" "primary-selection-unstable-v1-protocol"

# Idle Inhibit - Prevent screen sleep (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/idle-inhibit/idle-inhibit-unstable-v1.xml" "idle-inhibit-unstable-v1-protocol"

# Pointer Constraints - Mouse locking (GAMING)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml" "pointer-constraints-unstable-v1-protocol"

# Relative Pointer - Mouse relative movement (GAMING)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/relative-pointer/relative-pointer-unstable-v1.xml" "relative-pointer-unstable-v1-protocol"

# Pointer Gestures - Touchpad gestures (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/pointer-gestures/pointer-gestures-unstable-v1.xml" "pointer-gestures-unstable-v1-protocol"

# Keyboard Shortcuts Inhibit - Gaming/fullscreen (GAMING)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/keyboard-shortcuts-inhibit/keyboard-shortcuts-inhibit-unstable-v1.xml" "keyboard-shortcuts-inhibit-unstable-v1-protocol"

# XDG Foreign - Window embedding (IMPORTANT)
generate_protocol "$WAYLAND_PROTOCOLS/unstable/xdg-foreign/xdg-foreign-unstable-v2.xml" "xdg-foreign-unstable-v2-protocol"

# Input Timestamps - Precise input timing
generate_protocol "$WAYLAND_PROTOCOLS/unstable/input-timestamps/input-timestamps-unstable-v1.xml" "input-timestamps-unstable-v1-protocol"

# Fullscreen Shell - Simple fullscreen interface
generate_protocol "$WAYLAND_PROTOCOLS/unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml" "fullscreen-shell-unstable-v1-protocol"

# Linux Explicit Synchronization - GPU synchronization
generate_protocol "$WAYLAND_PROTOCOLS/unstable/linux-explicit-synchronization/linux-explicit-synchronization-unstable-v1.xml" "linux-explicit-synchronization-unstable-v1-protocol"

echo ""
echo "5️⃣  XWAYLAND PROTOCOLS (X11 app support)"
echo "======================================"

# XWayland Keyboard Grab - X11 key grabbing
generate_protocol "$WAYLAND_PROTOCOLS/unstable/xwayland-keyboard-grab/xwayland-keyboard-grab-unstable-v1.xml" "xwayland-keyboard-grab-unstable-v1-protocol"

# XWayland Shell - X11 window management
if [[ -f "$WAYLAND_PROTOCOLS/staging/xwayland-shell/xwayland-shell-v1.xml" ]]; then
    generate_protocol "$WAYLAND_PROTOCOLS/staging/xwayland-shell/xwayland-shell-v1.xml" "xwayland-shell-v1-protocol"
fi

echo ""
echo "6️⃣  WLR-SPECIFIC PROTOCOLS (wlroots extensions)"
echo "=========================================="

# Check for wlr-protocols
WLR_PROTOCOLS="/usr/share/wlr-protocols"
if [[ -d "$WLR_PROTOCOLS" ]]; then
    echo "📦 Found wlr-protocols, generating wlroots-specific protocols..."
    
    # Data Device Manager - Advanced clipboard
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-data-device-manager-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-data-device-manager-unstable-v1.xml" "wlr-data-device-manager-unstable-v1-protocol"
    fi
    
    # Export DMABUF - Screen sharing
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-export-dmabuf-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-export-dmabuf-unstable-v1.xml" "wlr-export-dmabuf-unstable-v1-protocol"
    fi
    
    # Screencopy - Screenshots
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-screencopy-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-screencopy-unstable-v1.xml" "wlr-screencopy-unstable-v1-protocol"
    fi
    
    # Gamma Control - Monitor gamma/brightness
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-gamma-control-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-gamma-control-unstable-v1.xml" "wlr-gamma-control-unstable-v1-protocol"
    fi
    
    # Output Power Management - Monitor power control
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-output-power-management-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-output-power-management-unstable-v1.xml" "wlr-output-power-management-unstable-v1-protocol"
    fi
    
    # Virtual Pointer - Remote desktop
    if [[ -f "$WLR_PROTOCOLS/unstable/wlr-virtual-pointer-unstable-v1.xml" ]]; then
        generate_protocol "$WLR_PROTOCOLS/unstable/wlr-virtual-pointer-unstable-v1.xml" "wlr-virtual-pointer-unstable-v1-protocol"
    fi
    
else
    echo "❌ wlr-protocols not found - install wlr-protocols for additional features"
fi

echo ""
echo "✅ PROTOCOL GENERATION COMPLETE!"
echo "================================"

# Count generated files
HEADER_COUNT=$(ls -1 $PROTOCOLS_DIR/*.h 2>/dev/null | wc -l)
SOURCE_COUNT=$(ls -1 $PROTOCOLS_DIR/*.c 2>/dev/null | wc -l)

echo "📊 Generated:"
echo "   • $HEADER_COUNT header files (.h)"
echo "   • $SOURCE_COUNT source files (.c)"
echo ""
echo "🎯 These protocols provide:"
echo "   • Window management (XDG Shell)"
echo "   • Hardware acceleration (DMA-BUF)" 
echo "   • HiDPI support (Fractional Scale)"
echo "   • Clipboard/primary selection"
echo "   • Input methods & text input"
echo "   • Gaming features (pointer lock, relative motion)"
echo "   • Desktop features (layer shell, screenshots)"
echo "   • Power management & idle detection"
echo "   • Security & sandboxing"
echo "   • X11 compatibility (XWayland)"
echo ""
echo "🚀 Ready to build a real Wayland compositor!"

