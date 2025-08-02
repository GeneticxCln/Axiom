#!/bin/bash

# Axiom Smart Updater Integration Script
# Seamlessly integrates the enhanced v2 updater with existing infrastructure

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SMART_UPDATER="$SCRIPT_DIR/axiom-smart-updater-v2.sh"
LEGACY_UPDATER="$SCRIPT_DIR/axiom-updater.sh"
AUTO_UPDATER="$SCRIPT_DIR/axiom-auto-updater.sh"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}🚀 Axiom Smart Updater Integration${NC}"
echo "======================================"

# Check if smart updater exists
if [ ! -f "$SMART_UPDATER" ]; then
    echo -e "${RED}❌ Error: Smart updater v2 not found at $SMART_UPDATER${NC}"
    exit 1
fi

# Make executable
chmod +x "$SMART_UPDATER"
echo -e "${GREEN}✅ Smart updater v2 is ready${NC}"

# Create integration wrapper
echo -e "${BLUE}📦 Creating integration wrapper...${NC}"

cat > "$SCRIPT_DIR/axiom-unified-updater.sh" << 'EOF'
#!/bin/bash

# Axiom Unified Updater - Intelligent update system with fallback support
# Automatically selects the best updater based on conditions

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SMART_UPDATER="$SCRIPT_DIR/axiom-smart-updater-v2.sh"
LEGACY_UPDATER="$SCRIPT_DIR/axiom-updater.sh"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Check system capabilities
check_smart_updater_support() {
    # Check for required commands
    local required_commands=("bc" "git" "pacman" "meson")
    
    for cmd in "${required_commands[@]}"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            log "${YELLOW}⚠️  Missing command: $cmd - falling back to legacy updater${NC}"
            return 1
        fi
    done
    
    # Check if we have enough disk space for advanced features
    local available_space=$(df "$SCRIPT_DIR" | tail -1 | awk '{print $4}')
    if [ "$available_space" -lt 1048576 ]; then  # Less than 1GB
        log "${YELLOW}⚠️  Low disk space - falling back to legacy updater${NC}"
        return 1
    fi
    
    return 0
}

# Intelligent updater selection
select_updater() {
    local mode=${1:-"auto"}
    
    case "$mode" in
        "smart"|"v2")
            if [ -f "$SMART_UPDATER" ]; then
                echo "$SMART_UPDATER"
            else
                log "${YELLOW}⚠️  Smart updater not found - using legacy${NC}"
                echo "$LEGACY_UPDATER"
            fi
            ;;
        "legacy"|"v1")
            echo "$LEGACY_UPDATER"
            ;;
        "auto")
            if check_smart_updater_support && [ -f "$SMART_UPDATER" ]; then
                log "${GREEN}🧠 Using smart updater v2${NC}"
                echo "$SMART_UPDATER"
            else
                log "${BLUE}📦 Using legacy updater v1${NC}"
                echo "$LEGACY_UPDATER"
            fi
            ;;
        *)
            log "${RED}❌ Unknown updater mode: $mode${NC}"
            exit 1
            ;;
    esac
}

# Convert legacy arguments to smart updater arguments
convert_arguments() {
    local selected_updater="$1"
    shift
    local args=("$@")
    
    if [[ "$selected_updater" == *"smart-updater"* ]]; then
        # Convert legacy arguments to smart updater format
        local smart_args=()
        
        for arg in "${args[@]}"; do
            case "$arg" in
                "--check-only")
                    smart_args+=("--risk-only")
                    ;;
                "--force")
                    export FORCE_UPDATE=true
                    smart_args+=("--smart-update")
                    ;;
                "--rebuild")
                    smart_args+=("--performance-test")
                    ;;
                "--backup-only")
                    # Legacy functionality - create backup without smart features
                    "$LEGACY_UPDATER" --backup-only
                    exit $?
                    ;;
                "--help")
                    smart_args+=("--help")
                    ;;
                *)
                    # Default to smart update
                    smart_args+=("--smart-update")
                    ;;
            esac
        done
        
        # If no arguments provided, default to smart update
        if [ ${#smart_args[@]} -eq 0 ]; then
            smart_args+=("--smart-update")
        fi
        
        echo "${smart_args[@]}"
    else
        # Pass through legacy arguments
        echo "${args[@]}"
    fi
}

# Main execution
main() {
    log "${CYAN}🚀 Axiom Unified Updater${NC}"
    
    # Parse special unified arguments
    local updater_mode="auto"
    local filtered_args=()
    
    for arg in "$@"; do
        case "$arg" in
            "--use-smart")
                updater_mode="smart"
                ;;
            "--use-legacy")
                updater_mode="legacy"
                ;;
            "--use-auto")
                updater_mode="auto"
                ;;
            *)
                filtered_args+=("$arg")
                ;;
        esac
    done
    
    # Select appropriate updater
    local selected_updater=$(select_updater "$updater_mode")
    
    # Convert arguments if needed
    local final_args=$(convert_arguments "$selected_updater" "${filtered_args[@]}")
    
    # Execute the selected updater
    log "${BLUE}Executing: $(basename "$selected_updater") $final_args${NC}"
    exec "$selected_updater" $final_args
}

# Show help
show_help() {
    cat << EOF
${CYAN}Axiom Unified Updater - Intelligent update system${NC}

${YELLOW}USAGE:${NC}
    $0 [UPDATER_MODE] [OPTIONS]

${YELLOW}UPDATER MODES:${NC}
    --use-smart     Force use of smart updater v2
    --use-legacy    Force use of legacy updater v1  
    --use-auto      Automatically select best updater (default)

${YELLOW}LEGACY OPTIONS (converted automatically):${NC}
    --check-only    Check for updates only
    --force         Force update regardless of warnings
    --rebuild       Rebuild without updating
    --backup-only   Create backup only
    --help          Show this help

${YELLOW}SMART UPDATER OPTIONS:${NC}
    --smart-update      Full intelligent update process
    --risk-only         Risk assessment only
    --performance-test  Performance testing only
    --ab-test          A/B testing only
    --analyze-patterns  Pattern analysis only
    --rollback         Smart rollback

${YELLOW}EXAMPLES:${NC}
    $0                      # Auto-select updater and run
    $0 --use-smart          # Force smart updater
    $0 --use-legacy --force # Force legacy updater with force flag
    $0 --check-only         # Check only (auto-converted)

EOF
}

# Handle help requests
if [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]] || [[ $# -eq 0 && -t 0 ]]; then
    show_help
    exit 0
fi

# Run main function
main "$@"
EOF

chmod +x "$SCRIPT_DIR/axiom-unified-updater.sh"
echo -e "${GREEN}✅ Created unified updater wrapper${NC}"

# Create systemd service override for smart updater
echo -e "${BLUE}⚙️  Creating systemd integration...${NC}"

cat > "$SCRIPT_DIR/axiom-smart-updater.service" << EOF
[Unit]
Description=Axiom Smart Updater v2 - Enhanced Intelligent Update System
After=network-online.target
Wants=network-online.target

[Service]
Type=oneshot
User=alex
Group=alex
WorkingDirectory=$SCRIPT_DIR
ExecStart=$SCRIPT_DIR/axiom-unified-updater.sh --use-smart --smart-update
StandardOutput=journal
StandardError=journal

# Enhanced security settings
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=false
ReadWritePaths=/home/alex/.axiom-updater.log /home/alex/.axiom-backups /home/alex/.axiom-performance.log /home/alex/.axiom-risk-assessment.log $SCRIPT_DIR
PrivateTmp=true
ProtectKernelTunables=true
ProtectKernelModules=true
ProtectControlGroups=true
RestrictRealtime=true
MemoryDenyWriteExecute=true
EOF

cat > "$SCRIPT_DIR/axiom-smart-updater.timer" << EOF
[Unit]
Description=Run Axiom Smart Updater twice daily
Requires=axiom-smart-updater.service

[Timer]
# Run twice daily at 9:00 AM and 6:00 PM
OnCalendar=*-*-* 09,18:00:00
Persistent=true
RandomizedDelaySec=3600

[Install]
WantedBy=timers.target
EOF

echo -e "${GREEN}✅ Created smart updater systemd files${NC}"

# Create configuration management
echo -e "${BLUE}🔧 Setting up configuration management...${NC}"

cat > "$SCRIPT_DIR/configure-smart-updater.sh" << 'EOF'
#!/bin/bash

# Axiom Smart Updater Configuration Manager

CONFIG_FILE="$HOME/.config/axiom-smart-updater.conf"
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

configure_interactive() {
    echo -e "${BLUE}🔧 Axiom Smart Updater Configuration${NC}"
    echo "====================================="
    
    # Load current config
    if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
    fi
    
    echo "Current settings:"
    echo "  Risk Assessment: ${ENABLE_RISK_ASSESSMENT:-true}"
    echo "  Performance Monitoring: ${ENABLE_PERFORMANCE_MONITORING:-true}"
    echo "  A/B Testing: ${ENABLE_AB_TESTING:-true}"
    echo "  ML Patterns: ${ENABLE_ML_PATTERNS:-true}"
    echo "  Max Risk Score: ${MAX_RISK_SCORE:-50}"
    echo
    
    # Interactive configuration
    read -p "Enable risk assessment? (y/N): " risk_assessment
    read -p "Enable performance monitoring? (y/N): " perf_monitoring
    read -p "Enable A/B testing? (y/N): " ab_testing
    read -p "Enable ML pattern analysis? (y/N): " ml_patterns
    read -p "Maximum acceptable risk score (0-100) [50]: " max_risk
    
    # Set defaults
    risk_assessment=${risk_assessment:-y}
    perf_monitoring=${perf_monitoring:-y}
    ab_testing=${ab_testing:-y}
    ml_patterns=${ml_patterns:-y}
    max_risk=${max_risk:-50}
    
    # Generate config
    mkdir -p "$(dirname "$CONFIG_FILE")"
    cat > "$CONFIG_FILE" << CONF
# Axiom Smart Updater v2 Configuration
ENABLE_RISK_ASSESSMENT=$([ "$risk_assessment" = "y" ] && echo "true" || echo "false")
ENABLE_PERFORMANCE_MONITORING=$([ "$perf_monitoring" = "y" ] && echo "true" || echo "false")
ENABLE_AB_TESTING=$([ "$ab_testing" = "y" ] && echo "true" || echo "false")
ENABLE_ML_PATTERNS=$([ "$ml_patterns" = "y" ] && echo "true" || echo "false")
MAX_RISK_SCORE=$max_risk
PERFORMANCE_REGRESSION_THRESHOLD=1.2
FORCE_UPDATE=false
CONF
    
    echo -e "${GREEN}✅ Configuration saved to $CONFIG_FILE${NC}"
}

case "${1:-interactive}" in
    "interactive"|"")
        configure_interactive
        ;;
    "reset")
        rm -f "$CONFIG_FILE"
        echo -e "${GREEN}✅ Configuration reset${NC}"
        ;;
    "show")
        if [ -f "$CONFIG_FILE" ]; then
            cat "$CONFIG_FILE"
        else
            echo "No configuration file found"
        fi
        ;;
    *)
        echo "Usage: $0 [interactive|reset|show]"
        ;;
esac
EOF

chmod +x "$SCRIPT_DIR/configure-smart-updater.sh"
echo -e "${GREEN}✅ Created configuration manager${NC}"

# Create installation script
echo -e "${BLUE}📦 Creating installation script...${NC}"

cat > "$SCRIPT_DIR/install-smart-updater.sh" << 'EOF'
#!/bin/bash

# Install Axiom Smart Updater v2 System

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}🚀 Installing Axiom Smart Updater v2${NC}"

# Install systemd files
echo -e "${BLUE}Installing systemd service and timer...${NC}"
sudo cp "$SCRIPT_DIR/axiom-smart-updater.service" /etc/systemd/system/
sudo cp "$SCRIPT_DIR/axiom-smart-updater.timer" /etc/systemd/system/

# Reload systemd
sudo systemctl daemon-reload

# Enable and start timer
sudo systemctl enable axiom-smart-updater.timer
sudo systemctl start axiom-smart-updater.timer

echo -e "${GREEN}✅ Smart updater installed!${NC}"
echo
echo -e "${YELLOW}Available commands:${NC}"
echo "  $SCRIPT_DIR/axiom-unified-updater.sh           # Intelligent updater selection"
echo "  $SCRIPT_DIR/axiom-smart-updater-v2.sh          # Direct smart updater access"
echo "  $SCRIPT_DIR/configure-smart-updater.sh         # Configure smart updater"
echo
echo -e "${YELLOW}Systemd management:${NC}"
echo "  sudo systemctl status axiom-smart-updater.timer   # Check timer status"
echo "  sudo systemctl stop axiom-smart-updater.timer     # Stop automatic updates"
echo "  sudo systemctl start axiom-smart-updater.timer    # Start automatic updates"
echo "  journalctl -u axiom-smart-updater.service         # View logs"
echo
echo -e "${YELLOW}Log files:${NC}"
echo "  ~/.axiom-updater.log           # Main log"
echo "  ~/.axiom-performance.log       # Performance metrics"
echo "  ~/.axiom-risk-assessment.log   # Risk assessments"
echo
echo -e "${GREEN}The system will now intelligently check for updates twice daily!${NC}"
EOF

chmod +x "$SCRIPT_DIR/install-smart-updater.sh"

# Summary
echo
echo -e "${GREEN}🎉 Integration Complete!${NC}"
echo "========================="
echo
echo -e "${YELLOW}What's been created:${NC}"
echo -e "  ${BLUE}📦 axiom-smart-updater-v2.sh${NC}     - Enhanced intelligent updater"
echo -e "  ${BLUE}🔗 axiom-unified-updater.sh${NC}      - Intelligent updater selection"
echo -e "  ${BLUE}⚙️  axiom-smart-updater.service${NC}  - Systemd service"
echo -e "  ${BLUE}⏰ axiom-smart-updater.timer${NC}     - Systemd timer (twice daily)"
echo -e "  ${BLUE}🔧 configure-smart-updater.sh${NC}    - Configuration manager"
echo -e "  ${BLUE}📦 install-smart-updater.sh${NC}      - Installation script"
echo
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Run configuration: ${CYAN}./configure-smart-updater.sh${NC}"
echo "2. Install system service: ${CYAN}./install-smart-updater.sh${NC}"
echo "3. Test the system: ${CYAN}./axiom-unified-updater.sh --check-only${NC}"
echo
echo -e "${YELLOW}Features added:${NC}"
echo -e "  🧠 ML-based risk assessment"
echo -e "  📊 Performance regression detection"
echo -e "  🧪 A/B testing framework"
echo -e "  🔄 Smart rollback system"
echo -e "  📈 Historical pattern analysis"
echo -e "  🎯 Enhanced compatibility matrix"
echo -e "  🤖 Intelligent update decisions"
echo
echo -e "${GREEN}Your Axiom intelligent update system is now ready!${NC}"
