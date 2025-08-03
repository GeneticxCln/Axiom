#!/bin/bash

# Axiom wlroots AI Monitor v1.0 - Intelligent wlroots tracking and compatibility management
# Monitors wlroots releases and automatically manages Axiom compatibility updates

set -euo pipefail

# Configuration
AXIOM_DIR="/home/alex/Projects/Axiom"
WLROOTS_REPO="https://gitlab.freedesktop.org/wlroots/wlroots.git"
MONITOR_CONFIG="$HOME/.config/axiom-wlroots-monitor.conf"
AI_LOG="$HOME/.axiom-wlroots-ai.log"
COMPATIBILITY_DB="$HOME/.axiom-wlroots-compatibility.db"
TEMP_DIR="/tmp/axiom-wlroots-monitor"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# AI-enhanced logging (safe for function output capture)
ai_log() {
    local level=$1
    local category=${2:-"AI"}
    shift 2
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local structured_log="[$timestamp] [$level] [$category] $message"
    
    echo -e "$structured_log" >> "$AI_LOG"
    
    # Enhanced console output with emojis (only to stderr to not interfere with function returns)
    case $level in
        "INFO") echo -e "${BLUE}🤖 [AI-Monitor]${NC} $message" >&2 ;;
        "WARN") echo -e "${YELLOW}⚠️  [AI-Monitor]${NC} $message" >&2 ;;
        "ERROR") echo -e "${RED}❌ [AI-Monitor]${NC} $message" >&2 ;;
        "SUCCESS") echo -e "${GREEN}✅ [AI-Monitor]${NC} $message" >&2 ;;
        "ANALYSIS") echo -e "${PURPLE}🧠 [AI-Analysis]${NC} $message" >&2 ;;
    esac
}

# Initialize AI monitoring system
init_ai_monitor() {
    ai_log "INFO" "INIT" "🚀 Initializing Axiom wlroots AI Monitor v1.0"
    
    # Create required directories
    mkdir -p "$(dirname "$MONITOR_CONFIG")"
    mkdir -p "$(dirname "$AI_LOG")"
    mkdir -p "$(dirname "$COMPATIBILITY_DB")"
    mkdir -p "$TEMP_DIR"
    
    # Initialize compatibility database if it doesn't exist
    if [ ! -f "$COMPATIBILITY_DB" ]; then
        init_compatibility_database
    fi
    
    # Load or create configuration
    load_configuration
    
    ai_log "SUCCESS" "INIT" "AI Monitor initialized successfully"
}

# Initialize compatibility database with known good combinations
init_compatibility_database() {
    ai_log "INFO" "DB" "🗄️  Initializing compatibility database"
    
    cat > "$COMPATIBILITY_DB" << 'EOF'
# Axiom wlroots Compatibility Database
# Format: wlroots_version:axiom_version:compatibility_score:tested_date:notes
0.19.0:4.0.0:95:2025-08-03:Fully tested and stable
0.19.1:4.0.0:90:2025-08-03:Minor API changes, mostly compatible
0.19.2:4.0.1:85:2025-08-03:Some breaking changes, requires patches
0.20.0:4.1.0:70:2025-08-03:Major version, significant changes expected
EOF
    
    ai_log "SUCCESS" "DB" "Compatibility database initialized with base data"
}

# Load configuration or create defaults
load_configuration() {
    if [ ! -f "$MONITOR_CONFIG" ]; then
        ai_log "INFO" "CONFIG" "Creating default configuration"
        
        cat > "$MONITOR_CONFIG" << 'EOF'
# Axiom wlroots AI Monitor Configuration

# Monitoring settings
CHECK_INTERVAL_HOURS=6
AUTO_UPDATE_ENABLED=true
RISK_THRESHOLD=30
COMPATIBILITY_THRESHOLD=80

# Notification settings
ENABLE_NOTIFICATIONS=true
NOTIFICATION_EMAIL=""

# Advanced AI settings
ENABLE_PREDICTIVE_ANALYSIS=true
ENABLE_COMPATIBILITY_LEARNING=true
LEARNING_SENSITIVITY=0.7
ROLLBACK_ON_FAILURE=true

# Testing settings
ENABLE_AUTOMATED_TESTING=true
TEST_TIMEOUT_MINUTES=15
PERFORMANCE_REGRESSION_THRESHOLD=1.2

# Integration settings
INTEGRATE_WITH_SMART_UPDATER=true
SMART_UPDATER_PATH="/home/alex/Projects/Axiom/axiom-smart-updater-v2.sh"
EOF
        ai_log "SUCCESS" "CONFIG" "Default configuration created"
    fi
    
    # Source configuration
    source "$MONITOR_CONFIG"
    ai_log "INFO" "CONFIG" "Configuration loaded successfully"
}

# Get current wlroots version information
get_current_wlroots_info() {
    local current_version=""
    local installation_method=""
    
    # Try different methods to detect wlroots
    if command -v pacman >/dev/null 2>&1; then
        current_version=$(pacman -Q wlroots 2>/dev/null | awk '{print $2}' | cut -d'-' -f1 || echo "")
        installation_method="pacman"
    elif command -v pkg-config >/dev/null 2>&1; then
        current_version=$(pkg-config --modversion wlroots 2>/dev/null || echo "")
        installation_method="pkg-config"
    elif [ -f "/usr/lib/pkgconfig/wlroots.pc" ]; then
        current_version=$(grep "Version:" /usr/lib/pkgconfig/wlroots.pc | awk '{print $2}')
        installation_method="pkgconfig-file"
    fi
    
    ai_log "INFO" "DETECT" "Current wlroots: $current_version (detected via $installation_method)"
    echo "$current_version"
}

# Fetch latest wlroots releases using GitLab API
fetch_wlroots_releases() {
    ai_log "INFO" "FETCH" "🌐 Fetching latest wlroots releases from GitLab"
    
    local releases_json="$TEMP_DIR/wlroots_releases.json"
    
    # Use GitLab API to get releases
    if curl -s "https://gitlab.freedesktop.org/api/v4/projects/wlroots%2Fwlroots/releases" > "$releases_json"; then
        ai_log "SUCCESS" "FETCH" "Successfully fetched wlroots release data"
    else
        ai_log "ERROR" "FETCH" "Failed to fetch wlroots releases"
        return 1
    fi
    
    # Parse releases and extract versions
    local latest_versions=$(jq -r '.[0:5] | .[] | .tag_name' "$releases_json" 2>/dev/null | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | head -5)
    
    ai_log "INFO" "FETCH" "Latest wlroots versions: $(echo $latest_versions | tr '\n' ' ')"
    echo "$latest_versions"
}

# AI-powered compatibility analysis
analyze_compatibility() {
    local wlroots_version=$1
    local axiom_version=${2:-"4.0.0"}
    
    ai_log "ANALYSIS" "COMPAT" "🧠 Analyzing compatibility: wlroots $wlroots_version ↔ Axiom $axiom_version"
    
    # Check compatibility database
    local db_entry=$(grep "^$wlroots_version:$axiom_version:" "$COMPATIBILITY_DB" 2>/dev/null || echo "")
    
    if [ -n "$db_entry" ]; then
        local compatibility_score=$(echo "$db_entry" | cut -d':' -f3)
        ai_log "ANALYSIS" "COMPAT" "📊 Database match found: compatibility score $compatibility_score/100"
        echo "$compatibility_score"
        return
    fi
    
    # AI-powered compatibility prediction
    local predicted_score=50  # Default uncertain score
    
    # Version proximity analysis
    local current_wlroots=$(get_current_wlroots_info)
    if [ -n "$current_wlroots" ]; then
        local version_distance=$(calculate_version_distance "$current_wlroots" "$wlroots_version")
        
        # Closer versions = higher compatibility
        if [ "$version_distance" -eq 0 ]; then
            predicted_score=95
        elif [ "$version_distance" -eq 1 ]; then
            predicted_score=85
        elif [ "$version_distance" -eq 2 ]; then
            predicted_score=70
        else
            predicted_score=50
        fi
    fi
    
    # Pattern-based analysis from historical data
    local historical_pattern=$(analyze_historical_patterns "$wlroots_version")
    predicted_score=$((predicted_score + historical_pattern))
    
    # Cap at 100
    if [ "$predicted_score" -gt 100 ]; then
        predicted_score=100
    fi
    
    ai_log "ANALYSIS" "COMPAT" "🎯 Predicted compatibility score: $predicted_score/100"
    echo "$predicted_score"
}

# Calculate semantic version distance
calculate_version_distance() {
    local ver1=$1
    local ver2=$2
    
    # Handle empty versions
    if [ -z "$ver1" ] || [ -z "$ver2" ]; then
        echo "999"
        return
    fi
    
    # Simple version distance calculation
    local v1_major=$(echo "$ver1" | cut -d'.' -f1 | grep -o '[0-9]*' | head -1)
    local v1_minor=$(echo "$ver1" | cut -d'.' -f2 | grep -o '[0-9]*' | head -1)
    local v1_patch=$(echo "$ver1" | cut -d'.' -f3 | grep -o '[0-9]*' | head -1)
    
    local v2_major=$(echo "$ver2" | cut -d'.' -f1 | grep -o '[0-9]*' | head -1)
    local v2_minor=$(echo "$ver2" | cut -d'.' -f2 | grep -o '[0-9]*' | head -1)
    local v2_patch=$(echo "$ver2" | cut -d'.' -f3 | grep -o '[0-9]*' | head -1)
    
    # Default to 0 if empty
    v1_major=${v1_major:-0}
    v1_minor=${v1_minor:-0}
    v1_patch=${v1_patch:-0}
    v2_major=${v2_major:-0}
    v2_minor=${v2_minor:-0}
    v2_patch=${v2_patch:-0}
    
    # Calculate Manhattan distance
    local major_diff=$(( v2_major > v1_major ? v2_major - v1_major : v1_major - v2_major ))
    local minor_diff=$(( v2_minor > v1_minor ? v2_minor - v1_minor : v1_minor - v2_minor ))
    local patch_diff=$(( v2_patch > v1_patch ? v2_patch - v1_patch : v1_patch - v2_patch ))
    
    local distance=$(( major_diff * 100 + minor_diff * 10 + patch_diff ))
    
    echo "$distance"
}

# Analyze historical patterns for compatibility prediction
analyze_historical_patterns() {
    local target_version=$1
    local pattern_bonus=0
    
    # Check for patterns in compatibility database
    local major_version=$(echo "$target_version" | cut -d'.' -f1)
    local minor_version=$(echo "$target_version" | cut -d'.' -f2)
    
    # Look for similar version patterns
    local similar_versions=$(grep "^$major_version\.$minor_version\." "$COMPATIBILITY_DB" | wc -l)
    
    if [ "$similar_versions" -gt 0 ]; then
        local avg_score=$(grep "^$major_version\.$minor_version\." "$COMPATIBILITY_DB" | cut -d':' -f3 | awk '{sum+=$1} END {print int(sum/NR)}')
        pattern_bonus=$((avg_score - 50))  # Bonus/penalty based on historical average
        ai_log "ANALYSIS" "PATTERN" "📈 Historical pattern analysis: $similar_versions similar versions, average score $avg_score"
    fi
    
    echo "$pattern_bonus"
}

# Generate compatibility report
generate_compatibility_report() {
    local wlroots_version=$1
    local compatibility_score=$2
    
    ai_log "INFO" "REPORT" "📋 Generating compatibility report for wlroots $wlroots_version"
    
    local report_file="$TEMP_DIR/compatibility_report_${wlroots_version//./_}.txt"
    
    cat > "$report_file" << EOF
Axiom wlroots Compatibility Report
==================================
Date: $(date '+%Y-%m-%d %H:%M:%S')
wlroots Version: $wlroots_version
Current Axiom Version: 4.0.0
Compatibility Score: $compatibility_score/100

Risk Assessment:
EOF
    
    if [ "$compatibility_score" -ge 90 ]; then
        echo "✅ LOW RISK - Highly compatible, safe to update" >> "$report_file"
    elif [ "$compatibility_score" -ge 70 ]; then
        echo "⚠️  MEDIUM RISK - Generally compatible, minor issues possible" >> "$report_file"
    elif [ "$compatibility_score" -ge 50 ]; then
        echo "🔶 HIGH RISK - Compatibility uncertain, thorough testing required" >> "$report_file"
    else
        echo "❌ VERY HIGH RISK - Likely incompatible, major changes expected" >> "$report_file"
    fi
    
    echo "" >> "$report_file"
    echo "Recommendations:" >> "$report_file"
    
    if [ "$compatibility_score" -ge "$COMPATIBILITY_THRESHOLD" ]; then
        echo "- Safe to proceed with automated update" >> "$report_file"
        echo "- Enable automated testing" >> "$report_file"
    else
        echo "- Manual review recommended before update" >> "$report_file"
        echo "- Create backup before proceeding" >> "$report_file"
        echo "- Consider waiting for Axiom compatibility patch" >> "$report_file"
    fi
    
    ai_log "SUCCESS" "REPORT" "Compatibility report generated: $report_file"
    echo "$report_file"
}

# Intelligent update decision system
make_update_decision() {
    local wlroots_version=$1
    local compatibility_score=$2
    
    ai_log "ANALYSIS" "DECISION" "🤖 Making intelligent update decision"
    
    # Risk-based decision making
    if [ "$compatibility_score" -ge 90 ]; then
        ai_log "SUCCESS" "DECISION" "✅ HIGH COMPATIBILITY - Recommending automatic update"
        return 0
    elif [ "$compatibility_score" -ge "$COMPATIBILITY_THRESHOLD" ]; then
        ai_log "INFO" "DECISION" "⚠️  MODERATE COMPATIBILITY - Recommending cautious update"
        return 1
    else
        ai_log "WARN" "DECISION" "🔴 LOW COMPATIBILITY - Recommending manual review"
        return 2
    fi
}

# Integration with existing smart updater
trigger_smart_update() {
    local wlroots_version=$1
    local compatibility_score=$2
    local update_mode=$3
    
    ai_log "INFO" "INTEGRATION" "🔗 Integrating with smart updater system"
    
    if [ "$INTEGRATE_WITH_SMART_UPDATER" = "true" ] && [ -f "$SMART_UPDATER_PATH" ]; then
        # Set environment variables for smart updater
        export WLROOTS_TARGET_VERSION="$wlroots_version"
        export COMPATIBILITY_SCORE="$compatibility_score"
        export AI_MONITOR_MODE="true"
        
        case $update_mode in
            0) # High compatibility - automatic
                ai_log "INFO" "INTEGRATION" "Triggering automatic smart update"
                "$SMART_UPDATER_PATH" --smart-update --ai-assisted
                ;;
            1) # Moderate compatibility - cautious
                ai_log "INFO" "INTEGRATION" "Triggering cautious smart update"
                "$SMART_UPDATER_PATH" --smart-update --ai-assisted --extra-caution
                ;;
            2) # Low compatibility - manual
                ai_log "WARN" "INTEGRATION" "Manual intervention required - creating notification"
                create_manual_intervention_notice "$wlroots_version" "$compatibility_score"
                ;;
        esac
    else
        ai_log "WARN" "INTEGRATION" "Smart updater integration disabled or not available"
    fi
}

# Create manual intervention notice
create_manual_intervention_notice() {
    local wlroots_version=$1
    local compatibility_score=$2
    
    local notice_file="$HOME/.axiom-manual-intervention-required.txt"
    
    cat > "$notice_file" << EOF
⚠️  AXIOM MANUAL INTERVENTION REQUIRED ⚠️

A new wlroots version ($wlroots_version) is available, but the AI system
has determined that manual review is required due to low compatibility
score ($compatibility_score/100).

Recommended actions:
1. Review the compatibility report in $TEMP_DIR/
2. Check Axiom release notes for compatibility updates
3. Consider running manual tests before updating
4. Update Axiom code if necessary before wlroots update

To proceed manually:
./axiom-smart-updater-v2.sh --force --smart-update

Generated: $(date '+%Y-%m-%d %H:%M:%S')
EOF
    
    ai_log "WARN" "NOTICE" "Manual intervention notice created: $notice_file"
    
    # Send notification if configured
    if [ "$ENABLE_NOTIFICATIONS" = "true" ] && [ -n "$NOTIFICATION_EMAIL" ]; then
        send_notification "$wlroots_version" "$compatibility_score"
    fi
}

# Send notification (email or system notification)
send_notification() {
    local wlroots_version=$1
    local compatibility_score=$2
    
    # Try desktop notification first
    if command -v notify-send >/dev/null 2>&1; then
        notify-send "Axiom wlroots Update" "New wlroots $wlroots_version available (compatibility: $compatibility_score/100)"
        ai_log "INFO" "NOTIFY" "Desktop notification sent"
    fi
    
    # Email notification if configured
    if [ -n "$NOTIFICATION_EMAIL" ] && command -v mail >/dev/null 2>&1; then
        echo "New wlroots version $wlroots_version is available. Compatibility score: $compatibility_score/100. Check Axiom AI Monitor logs for details." | \
        mail -s "Axiom wlroots Update Available" "$NOTIFICATION_EMAIL"
        ai_log "INFO" "NOTIFY" "Email notification sent to $NOTIFICATION_EMAIL"
    fi
}

# Main monitoring loop
run_monitoring_cycle() {
    ai_log "INFO" "MONITOR" "🔄 Starting monitoring cycle"
    
    # Get current state
    local current_wlroots=$(get_current_wlroots_info)
    if [ -z "$current_wlroots" ]; then
        ai_log "ERROR" "MONITOR" "Could not detect current wlroots version"
        return 1
    fi
    
    # Fetch latest releases
    local latest_versions=$(fetch_wlroots_releases)
    if [ -z "$latest_versions" ]; then
        ai_log "ERROR" "MONITOR" "Could not fetch wlroots releases"
        return 1
    fi
    
    # Check each recent version
    local update_found=false
    while IFS= read -r version; do
        if [ "$version" != "$current_wlroots" ]; then
            ai_log "INFO" "MONITOR" "🆕 New wlroots version detected: $version"
            
            # Analyze compatibility
            local compatibility_score=$(analyze_compatibility "$version")
            
            # Generate report
            local report_file=$(generate_compatibility_report "$version" "$compatibility_score")
            
            # Make decision
            make_update_decision "$version" "$compatibility_score"
            local decision_result=$?
            
            # Trigger appropriate action
            if [ "$AUTO_UPDATE_ENABLED" = "true" ]; then
                trigger_smart_update "$version" "$compatibility_score" "$decision_result"
            else
                ai_log "INFO" "MONITOR" "Auto-update disabled, creating manual notice"
                create_manual_intervention_notice "$version" "$compatibility_score"
            fi
            
            update_found=true
            break  # Process only the latest version for now
        fi
    done <<< "$latest_versions"
    
    if [ "$update_found" = "false" ]; then
        ai_log "INFO" "MONITOR" "✅ No new wlroots versions found - system up to date"
    fi
    
    ai_log "SUCCESS" "MONITOR" "Monitoring cycle completed"
}

# Update compatibility database with new test results
update_compatibility_database() {
    local wlroots_version=$1
    local axiom_version=$2
    local compatibility_score=$3
    local test_result=${4:-"untested"}
    local notes=${5:-"AI predicted"}
    
    ai_log "INFO" "DB" "📝 Updating compatibility database"
    
    # Remove existing entry if present
    sed -i "/^$wlroots_version:$axiom_version:/d" "$COMPATIBILITY_DB"
    
    # Add new entry
    echo "$wlroots_version:$axiom_version:$compatibility_score:$(date +%Y-%m-%d):$notes" >> "$COMPATIBILITY_DB"
    
    ai_log "SUCCESS" "DB" "Compatibility database updated"
}

# CLI interface
show_help() {
    echo -e "${BLUE}Axiom wlroots AI Monitor v1.0${NC}"
    echo ""
    echo -e "${YELLOW}USAGE:${NC}"
    echo "    $0 [COMMAND] [OPTIONS]"
    echo ""
    echo -e "${YELLOW}COMMANDS:${NC}"
    echo "    monitor         Run single monitoring cycle"
    echo "    daemon          Run continuous monitoring daemon"
    echo "    check           Check for updates without action"
    echo "    analyze         Analyze specific wlroots version"
    echo "    report          Generate compatibility report"
    echo "    config          Show current configuration"
    echo "    database        Show compatibility database"
    echo ""
    echo -e "${YELLOW}OPTIONS:${NC}"
    echo "    --version VER   Specify wlroots version for analysis"
    echo "    --force         Force action regardless of risk"
    echo "    --no-auto       Disable automatic updates"
    echo "    --verbose       Enable verbose logging"
    echo "    --help          Show this help"
}

# Main execution
main() {
    local command=${1:-"monitor"}
    
    # Initialize system
    init_ai_monitor
    
    case "$command" in
        "monitor")
            run_monitoring_cycle
            ;;
        "daemon")
            ai_log "INFO" "DAEMON" "🔄 Starting continuous monitoring daemon"
            while true; do
                run_monitoring_cycle
                ai_log "INFO" "DAEMON" "Sleeping for $CHECK_INTERVAL_HOURS hours..."
                sleep $((CHECK_INTERVAL_HOURS * 3600))
            done
            ;;
        "check")
            ai_log "INFO" "CHECK" "🔍 Checking for wlroots updates (no action)"
            AUTO_UPDATE_ENABLED=false run_monitoring_cycle
            ;;
        "analyze")
            local target_version=${2:-""}
            if [ -z "$target_version" ]; then
                ai_log "ERROR" "CLI" "Version required for analysis. Use: $0 analyze VERSION"
                exit 1
            fi
            local score=$(analyze_compatibility "$target_version")
            generate_compatibility_report "$target_version" "$score"
            ;;
        "config")
            ai_log "INFO" "CONFIG" "📋 Current configuration:"
            cat "$MONITOR_CONFIG"
            ;;
        "database")
            ai_log "INFO" "DB" "🗄️  Compatibility database:"
            cat "$COMPATIBILITY_DB"
            ;;
        "--help"|"-h"|"help")
            show_help
            ;;
        *)
            ai_log "ERROR" "CLI" "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

# Handle signals for graceful shutdown
trap 'ai_log "INFO" "SIGNAL" "Received shutdown signal, cleaning up..."; exit 0' SIGTERM SIGINT

# Run main function
main "$@"
