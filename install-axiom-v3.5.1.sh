#!/bin/bash

# Axiom Wayland Compositor v3.5.1 - Security Release Installation Script
# This script installs the secure, production-ready version of Axiom

set -e

echo "🔒 Installing Axiom Wayland Compositor v3.5.1 - Security Release"
echo "=================================================================="

# Check for required dependencies
echo "📋 Checking dependencies..."
if ! command -v meson &> /dev/null; then
    echo "❌ Error: meson is required but not installed"
    echo "   Install with: sudo pacman -S meson (Arch) or sudo apt install meson (Ubuntu)"
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo "❌ Error: ninja is required but not installed"
    echo "   Install with: sudo pacman -S ninja (Arch) or sudo apt install ninja-build (Ubuntu)"
    exit 1
fi

echo "✅ Dependencies satisfied"

# Build Axiom
echo "🔨 Building Axiom v3.5.1..."
if [ ! -d "build" ]; then
    meson setup build
fi

ninja -C build

echo "🧪 Running tests..."
ninja -C build test

if [ $? -eq 0 ]; then
    echo "✅ All tests passed!"
else
    echo "❌ Tests failed. Installation aborted."
    exit 1
fi

# Install binary and session script
echo "📦 Installing Axiom binary and session script..."
sudo cp build/axiom /usr/local/bin/axiom
sudo chmod +x /usr/local/bin/axiom
sudo cp axiom-session /usr/local/bin/axiom-session
sudo chmod +x /usr/local/bin/axiom-session

# Install desktop session files
echo "🖥️ Installing desktop session files..."
sudo mkdir -p /usr/share/wayland-sessions
sudo cp axiom.desktop /usr/share/wayland-sessions/axiom.desktop
sudo cp axiom.session /usr/share/wayland-sessions/axiom.session

echo ""
echo "🎉 Axiom v3.5.1 Security Release installed successfully!"
echo ""
echo "🔒 Security improvements in this release:"
echo "   • Fixed critical buffer overflow vulnerability"
echo "   • Resolved undefined function usage"
echo "   • Enhanced memory safety with comprehensive error handling"
echo "   • Added null pointer validations throughout effects system"
echo ""
echo "🚀 Usage:"
echo "   • Select 'Axiom' from your display manager login screen"
echo "   • Or run directly: axiom"
echo ""
echo "📚 Documentation: https://github.com/GeneticxCln/Axiom"
echo "🐛 Report issues: https://github.com/GeneticxCln/Axiom/issues"
