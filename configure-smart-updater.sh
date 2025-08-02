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
