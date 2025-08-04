#!/bin/bash

echo "Testing Axiom native mode startup..."

# Stop current desktop session first
echo "Stopping current session..."
pkill -f Hyprland 2>/dev/null || true
sleep 2

# Try to run axiom in native mode briefly
echo "Starting Axiom in native mode..."
timeout 5s ./build/axiom --no-nested 2>&1 | tee native_test.log

echo "Exit code: $?"
echo "Log contents:"
cat native_test.log
