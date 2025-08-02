#!/bin/bash

# Axiom Auto-Updater Setup Script
# Configures and installs the intelligent auto-updater for Axiom

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
UPDATER_SCRIPT="$SCRIPT_DIR/axiom-auto-updater.sh"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${BLUE}🚀 Axiom Auto-Updater Setup${NC}"
echo "=================================="

# Check if the auto-updater script exists
if [ ! -f "$UPDATER_SCRIPT" ]; then
    echo -e "${RED}❌ Error: axiom-auto-updater.sh not found in current directory${NC}"
    exit 1
fi

# Make sure it's executable
chmod +x "$UPDATER_SCRIPT"

echo -e "${GREEN}✅ Auto-updater script found and made executable${NC}"

# Initial configuration
echo -e "${YELLOW}📝 Initial Configuration${NC}"
echo "Would you like to:"
echo "1. Install as system service (recommended for servers)"
echo "2. Set up manual monitoring (run when needed)"
echo "3. Configure and run once"

read -p "Choose option (1-3): " choice

case $choice in
    1)
        echo -e "${BLUE}🔧 Installing as system service...${NC}"
        
        # Run the installation
        "$UPDATER_SCRIPT" --install
        
        echo ""
        echo -e "${GREEN}✅ Auto-updater installed as system service!${NC}"
        echo ""
        echo -e "${YELLOW}Service Management:${NC}"
        echo "  Status: sudo systemctl status axiom-auto-updater"
        echo "  Stop:   sudo systemctl stop axiom-auto-updater"
        echo "  Start:  sudo systemctl start axiom-auto-updater"
        echo "  Logs:   sudo journalctl -u axiom-auto-updater -f"
        ;;
        
    2)
        echo -e "${BLUE}🔧 Setting up for manual use...${NC}"
        
        # Create configuration
        "$UPDATER_SCRIPT" --status
        
        echo ""
        echo -e "${GREEN}✅ Auto-updater configured for manual use!${NC}"
        echo ""
        echo -e "${YELLOW}Usage:${NC}"
        echo "  Check for updates: $UPDATER_SCRIPT --check"
        echo "  Force update:      $UPDATER_SCRIPT --update"
        echo "  Show status:       $UPDATER_SCRIPT --status"
        echo "  Edit config:       $UPDATER_SCRIPT --config"
        ;;
        
    3)
        echo -e "${BLUE}🔧 Running configuration and single update...${NC}"
        
        # Check status first to create config
        "$UPDATER_SCRIPT" --status
        
        echo ""
        read -p "Run update check now? (y/N): " run_check
        if [[ $run_check =~ ^[Yy]$ ]]; then
            "$UPDATER_SCRIPT" --check
        fi
        
        echo ""
        echo -e "${GREEN}✅ Configuration complete!${NC}"
        ;;
        
    *)
        echo -e "${RED}❌ Invalid choice${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${BLUE}📋 Configuration File:${NC} ~/.config/axiom-updater.conf"
echo -e "${BLUE}📊 Log File:${NC} /var/log/axiom-auto-updater.log"
echo ""
echo -e "${YELLOW}💡 Tips:${NC}"
echo "• Edit ~/.config/axiom-updater.conf to customize behavior"
echo "• Set AUTO_UPDATE=false to get notifications without auto-updating"
echo "• Adjust CHECK_INTERVAL to change monitoring frequency"
echo "• Enable/disable backups with BACKUP_BEFORE_UPDATE"
echo ""
echo -e "${GREEN}🎉 Axiom Auto-Updater setup complete!${NC}"
