#!/bin/bash

# Install Axiom Updater Automation

set -euo pipefail

GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}🚀 Installing Axiom Updater Automation${NC}"

# Copy systemd files
echo -e "${BLUE}Installing systemd service and timer...${NC}"
sudo cp axiom-updater.service /etc/systemd/system/
sudo cp axiom-updater.timer /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable and start timer
sudo systemctl enable axiom-updater.timer
sudo systemctl start axiom-updater.timer

echo -e "${GREEN}✅ Installation complete!${NC}"
echo
echo -e "${YELLOW}Available commands:${NC}"
echo "  ./axiom-updater.sh --check-only    # Check for updates manually"
echo "  ./axiom-updater.sh                 # Run full update process"
echo "  ./axiom-updater.sh --backup-only   # Create backup only"
echo "  ./axiom-updater.sh --force         # Force update ignoring warnings"
echo
echo -e "${YELLOW}Systemd management:${NC}"
echo "  sudo systemctl status axiom-updater.timer   # Check timer status"
echo "  sudo systemctl stop axiom-updater.timer     # Stop automatic checks"
echo "  sudo systemctl start axiom-updater.timer    # Start automatic checks"
echo "  journalctl -u axiom-updater.service         # View logs"
echo
echo -e "${YELLOW}Files created:${NC}"
echo "  ./axiom-updater.sh           # Main updater script"
echo "  /etc/systemd/system/axiom-updater.service"
echo "  /etc/systemd/system/axiom-updater.timer"
echo "  ~/.axiom-updater.log         # Log file"
echo "  ~/.axiom-backups/            # Backup directory"
echo
echo -e "${GREEN}The system will now check for Axiom updates daily at 9:00 AM${NC}"
echo -e "${GREEN}You'll receive notifications in the journal if updates are available${NC}"
