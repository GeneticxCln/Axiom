#!/bin/bash

echo "🎬 Axiom Animation System Demo"
echo "================================"

# Check if axiom binary exists
if [ ! -f "./build/axiom" ]; then
    echo "❌ Error: axiom binary not found. Please run 'ninja -C build' first."
    exit 1
fi

echo "🚀 Starting Axiom Compositor with Animation System..."
echo "   Features available:"
echo "   • Window appear animations (fade-in)"
echo "   • Focus ring animations (pulsing effect)"
echo "   • Window movement animations"
echo "   • Layout change animations"
echo "   • Real-time animation updates in main loop"
echo ""
echo "📋 Key bindings to test animations:"
echo "   • Super+Return: Open terminal (watch appear animation)"
echo "   • Alt+Tab: Cycle windows (watch focus ring animation)"
echo "   • Super+L: Change layouts (watch layout animations)"
echo "   • Super+Space: Toggle floating/tiled (watch transition)"
echo "   • Super+H/J: Adjust master ratio (watch resize animations)"
echo "   • Super+1-9: Switch workspaces"
echo ""
echo "🔧 To see animation debug output, check the logs for:"
echo "   [INFO] Started window appear animation"
echo "   [INFO] Started focus ring animation"
echo "   [DEBUG] Window move animation: (x, y)"
echo "   [DEBUG] Focus ring animation: intensity"
echo ""
echo "⚠️  Note: This compositor runs in nested mode for testing."
echo "   Visual animations depend on the underlying Wayland implementation."
echo ""

# Add execute permission
chmod +x ./build/axiom

# Run the compositor
echo "🎯 Launching compositor..."
# Set up environment for nested Wayland session
export WLR_BACKENDS=wayland
export WLR_WL_OUTPUTS=1
export WLR_RENDERER=pixman

# Try with software rendering to avoid graphics driver issues
./build/axiom
