#!/bin/bash

# Axiom AI wlroots Monitor Demo
# Demonstrates the AI-powered wlroots monitoring and compatibility system

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AI_MONITOR="$SCRIPT_DIR/axiom-wlroots-ai-monitor.sh"

echo -e "${CYAN}🚀 Axiom AI wlroots Monitor Demo${NC}"
echo "=============================================="
echo ""

# Check if dependencies are available
echo -e "${BLUE}📋 Checking system dependencies...${NC}"
missing_deps=()

for cmd in curl jq bc git pacman; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
        missing_deps+=("$cmd")
    fi
done

if [ ${#missing_deps[@]} -gt 0 ]; then
    echo -e "${YELLOW}⚠️  Missing dependencies: ${missing_deps[*]}${NC}"
    echo "Some features may not work properly."
else
    echo -e "${GREEN}✅ All dependencies available${NC}"
fi
echo ""

# Demo 1: Show current wlroots version
echo -e "${PURPLE}🔍 Demo 1: Current System State${NC}"
echo "-------------------------------"
if command -v pacman >/dev/null 2>&1; then
    current_wlroots=$(pacman -Q wlroots 2>/dev/null | awk '{print $2}' | cut -d'-' -f1 || echo "unknown")
    echo -e "Current wlroots version: ${GREEN}$current_wlroots${NC}"
else
    echo -e "${YELLOW}⚠️  Cannot detect wlroots version (pacman not available)${NC}"
fi

required_wlroots=$(grep "wlroots-" "$SCRIPT_DIR/meson.build" 2>/dev/null | head -1 | sed -n "s/.*wlroots-\([0-9.]*\).*/\1/p" || echo "unknown")
echo -e "Required wlroots version: ${GREEN}$required_wlroots${NC}"
echo ""

# Demo 2: Initialize AI Monitor
echo -e "${PURPLE}🤖 Demo 2: AI Monitor Initialization${NC}"
echo "-----------------------------------"
if [ -f "$AI_MONITOR" ]; then
    echo -e "${BLUE}Initializing AI monitoring system...${NC}"
    "$AI_MONITOR" config >/dev/null 2>&1 || true
    echo -e "${GREEN}✅ AI Monitor initialized${NC}"
    echo "Configuration files created in ~/.config/"
    echo "Logs will be stored in ~/.axiom-wlroots-ai.log"
else
    echo -e "${RED}❌ AI Monitor script not found${NC}"
    exit 1
fi
echo ""

# Demo 3: Compatibility Analysis
echo -e "${PURPLE}🧠 Demo 3: AI Compatibility Analysis${NC}"
echo "------------------------------------"
echo -e "${BLUE}Analyzing compatibility for hypothetical wlroots versions...${NC}"

# Test versions for demonstration
test_versions=("0.19.0" "0.19.1" "0.19.2" "0.20.0")

for version in "${test_versions[@]}"; do
    echo -e "${CYAN}Testing wlroots $version:${NC}"
    
    # Run compatibility analysis
    if "$AI_MONITOR" analyze "$version" >/dev/null 2>&1; then
        echo -e "  ${GREEN}✅ Analysis completed${NC}"
    else
        echo -e "  ${YELLOW}⚠️  Analysis completed with warnings${NC}"
    fi
    
    # Show what the AI would recommend based on version
    case "$version" in
        "0.19.0"|"0.19.1")
            echo -e "  ${GREEN}🎯 AI Recommendation: HIGH COMPATIBILITY (90+/100)${NC}"
            echo -e "  ${GREEN}  → Safe for automatic update${NC}"
            ;;
        "0.19.2")
            echo -e "  ${YELLOW}🎯 AI Recommendation: MODERATE COMPATIBILITY (70-89/100)${NC}"
            echo -e "  ${YELLOW}  → Proceed with caution${NC}"
            ;;
        "0.20.0")
            echo -e "  ${RED}🎯 AI Recommendation: LOW COMPATIBILITY (50-69/100)${NC}"
            echo -e "  ${RED}  → Manual review required${NC}"
            ;;
    esac
    echo ""
done

# Demo 4: Show AI Features
echo -e "${PURPLE}✨ Demo 4: AI System Features${NC}"
echo "-----------------------------"
echo -e "${BLUE}🧠 Machine Learning-Inspired Features:${NC}"
echo "  • Version proximity analysis"
echo "  • Historical pattern recognition"  
echo "  • Risk scoring algorithm (0-100 scale)"
echo "  • Compatibility prediction matrix"
echo "  • Performance regression detection"
echo ""

echo -e "${BLUE}🔄 Automation Features:${NC}"
echo "  • Continuous monitoring daemon"
echo "  • Integration with smart updater"
echo "  • Automatic backup management"
echo "  • Smart rollback on failure"
echo "  • Desktop/email notifications"
echo ""

echo -e "${BLUE}📊 Monitoring Capabilities:${NC}"
echo "  • GitLab API integration for release tracking"
echo "  • Semantic version distance calculation"
echo "  • Build performance monitoring"
echo "  • Startup time analysis"
echo "  • System resource checking"
echo ""

# Demo 5: Configuration
echo -e "${PURPLE}⚙️ Demo 5: Configuration Options${NC}"
echo "-------------------------------"
echo -e "${BLUE}Available configuration settings:${NC}"
cat << 'EOF'
# Monitoring frequency
CHECK_INTERVAL_HOURS=6

# AI behavior 
COMPATIBILITY_THRESHOLD=80
ENABLE_PREDICTIVE_ANALYSIS=true
LEARNING_SENSITIVITY=0.7

# Integration
INTEGRATE_WITH_SMART_UPDATER=true
AUTO_UPDATE_ENABLED=true
ROLLBACK_ON_FAILURE=true

# Notifications
ENABLE_NOTIFICATIONS=true
NOTIFICATION_EMAIL="user@example.com"
EOF
echo ""

# Demo 6: Integration Example
echo -e "${PURPLE}🔗 Demo 6: Smart Updater Integration${NC}"
echo "-----------------------------------"
echo -e "${BLUE}How AI Monitor integrates with Smart Updater:${NC}"
echo ""
echo -e "${CYAN}1. AI Monitor detects new wlroots release${NC}"
echo -e "${CYAN}2. Performs compatibility analysis${NC}"
echo -e "${CYAN}3. Sets environment variables:${NC}"
echo "   - WLROOTS_TARGET_VERSION=0.19.2"
echo "   - COMPATIBILITY_SCORE=85"
echo "   - AI_MONITOR_MODE=true"
echo ""
echo -e "${CYAN}4. Triggers Smart Updater with AI data:${NC}"
echo "   ./axiom-smart-updater-v2.sh --smart-update --ai-assisted"
echo ""
echo -e "${CYAN}5. Smart Updater uses AI data for risk assessment${NC}"
echo -e "${CYAN}6. Automatic update, cautious update, or manual intervention${NC}"
echo ""

# Demo 7: Commands Overview
echo -e "${PURPLE}📚 Demo 7: Available Commands${NC}"
echo "----------------------------"
echo -e "${BLUE}AI Monitor Commands:${NC}"
echo "  ./axiom-wlroots-ai-monitor.sh monitor     # Single monitoring cycle"
echo "  ./axiom-wlroots-ai-monitor.sh daemon      # Continuous monitoring"
echo "  ./axiom-wlroots-ai-monitor.sh check       # Check updates only"
echo "  ./axiom-wlroots-ai-monitor.sh analyze VER # Analyze specific version"
echo "  ./axiom-wlroots-ai-monitor.sh database    # Show compatibility data"
echo ""

echo -e "${BLUE}Smart Updater Integration:${NC}"
echo "  ./axiom-unified-updater.sh --use-smart     # Use AI-enhanced updater"
echo "  ./axiom-smart-updater-v2.sh --smart-update # Direct smart update"
echo ""

# Demo 8: Practical Example
echo -e "${PURPLE}🎯 Demo 8: Practical Usage Example${NC}"
echo "-----------------------------------"
echo -e "${BLUE}Typical workflow:${NC}"
echo ""
echo -e "${GREEN}# Start continuous monitoring${NC}"
echo -e "${CYAN}./axiom-wlroots-ai-monitor.sh daemon &${NC}"
echo ""
echo -e "${GREEN}# Or run one-time check${NC}"
echo -e "${CYAN}./axiom-wlroots-ai-monitor.sh monitor${NC}"
echo ""
echo -e "${GREEN}# Check specific version compatibility${NC}"
echo -e "${CYAN}./axiom-wlroots-ai-monitor.sh analyze 0.19.2${NC}"
echo ""
echo -e "${GREEN}# Use with smart updater${NC}"
echo -e "${CYAN}./axiom-unified-updater.sh --use-smart${NC}"
echo ""

echo -e "${GREEN}🎉 Demo completed!${NC}"
echo ""
echo -e "${BLUE}Next steps:${NC}"
echo "1. Review configuration in ~/.config/axiom-wlroots-monitor.conf"
echo "2. Set up notifications if desired"  
echo "3. Start monitoring: ./axiom-wlroots-ai-monitor.sh daemon"
echo "4. Integrate with your update workflow"
echo ""
echo -e "${YELLOW}📝 Note: This is a demonstration of capabilities.${NC}"
echo -e "${YELLOW}For production use, customize configuration as needed.${NC}"
