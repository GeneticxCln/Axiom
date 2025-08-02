#!/bin/bash

# Axiom Smart Updater v2.0 - Enhanced Intelligent Update System
# Features: ML-based risk assessment, performance monitoring, advanced rollback

set -euo pipefail

# Configuration
AXIOM_DIR="/home/alex/Projects /Axiom"
LOG_FILE="$HOME/.axiom-updater.log"
BACKUP_DIR="$HOME/.axiom-backups"
PERFORMANCE_LOG="$HOME/.axiom-performance.log"
RISK_ASSESSMENT_LOG="$HOME/.axiom-risk-assessment.log"
CONFIG_FILE="$HOME/.config/axiom-smart-updater.conf"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Enhanced logging with structured data
log() {
    local level=$1
    local category=${2:-"GENERAL"}
    shift 2
    local message="$@"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    local structured_log="[$timestamp] [$level] [$category] $message"
    
    echo -e "$structured_log" | tee -a "$LOG_FILE"
    
    # Add to performance log if relevant
    if [[ "$category" == "PERFORMANCE" ]]; then
        echo "$structured_log" >> "$PERFORMANCE_LOG"
    fi
    
    # Add to risk assessment log if relevant
    if [[ "$category" == "RISK" ]]; then
        echo "$structured_log" >> "$RISK_ASSESSMENT_LOG"
    fi
}

# Performance monitoring functions
measure_build_time() {
    local start_time=$(date +%s)
    "$@"
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    log "INFO" "PERFORMANCE" "Build completed in ${duration}s"
    echo "$duration" > /tmp/axiom_build_time.tmp
}

measure_startup_time() {
    log "INFO" "PERFORMANCE" "Measuring Axiom startup time..."
    
    local start_time=$(date +%s.%N)
    timeout 10s "$AXIOM_DIR/build/axiom" --help >/dev/null 2>&1 || true
    local end_time=$(date +%s.%N)
    
    local startup_time=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
    log "INFO" "PERFORMANCE" "Startup time: ${startup_time}s"
    echo "$startup_time" > /tmp/axiom_startup_time.tmp
}

# Risk assessment system
assess_update_risk() {
    local risk_score=0
    local risk_factors=()
    
    log "INFO" "RISK" "🧠 Performing intelligent risk assessment..."
    
    # Check commit message patterns for risk indicators
    cd "$AXIOM_DIR"
    local recent_commits=$(git log --oneline -5 HEAD..origin/main)
    
    if echo "$recent_commits" | grep -qi "break\|deprecat\|remov\|unsafe"; then
        risk_score=$((risk_score + 30))
        risk_factors+=("Breaking changes detected in recent commits")
    fi
    
    if echo "$recent_commits" | grep -qi "experiment\|alpha\|beta\|test"; then
        risk_score=$((risk_score + 20))
        risk_factors+=("Experimental features in recent commits")
    fi
    
    # Check system load and resources
    local cpu_load=$(uptime | awk -F'load average:' '{print $2}' | awk '{print $1}' | sed 's/,//')
    if (( $(echo "$cpu_load > 2.0" | bc -l) )); then
        risk_score=$((risk_score + 15))
        risk_factors+=("High system CPU load: $cpu_load")
    fi
    
    # Check available disk space
    local disk_usage=$(df "$AXIOM_DIR" | tail -1 | awk '{print $5}' | sed 's/%//')
    if [ "$disk_usage" -gt 90 ]; then
        risk_score=$((risk_score + 25))
        risk_factors+=("Low disk space: ${disk_usage}% used")
    fi
    
    # Check if this is a work day (higher risk during business hours)
    local hour=$(date +%H)
    local day=$(date +%u)  # 1-7, Monday is 1
    if [ "$day" -le 5 ] && [ "$hour" -ge 9 ] && [ "$hour" -le 17 ]; then
        risk_score=$((risk_score + 10))
        risk_factors+=("Update during business hours")
    fi
    
    # Check recent update frequency (too many updates = higher risk)
    local recent_updates=$(grep "SUCCESS.*update completed" "$LOG_FILE" | tail -5 | wc -l)
    if [ "$recent_updates" -gt 3 ]; then
        risk_score=$((risk_score + 15))
        risk_factors+=("High update frequency detected")
    fi
    
    log "INFO" "RISK" "Risk assessment complete. Score: $risk_score/100"
    
    for factor in "${risk_factors[@]}"; do
        log "WARN" "RISK" "  🚨 $factor"
    done
    
    echo "$risk_score"
}

# Intelligent update decision system
make_update_decision() {
    local risk_score=$1
    local force_update=${2:-false}
    
    log "INFO" "DECISION" "🤖 Making intelligent update decision..."
    
    if [ "$force_update" = "true" ]; then
        log "INFO" "DECISION" "Force update requested - proceeding regardless of risk"
        return 0
    fi
    
    if [ "$risk_score" -le 20 ]; then
        log "INFO" "DECISION" "✅ LOW RISK ($risk_score/100) - Proceeding with update"
        return 0
    elif [ "$risk_score" -le 50 ]; then
        log "WARN" "DECISION" "⚠️  MEDIUM RISK ($risk_score/100) - Requesting user confirmation"
        read -p "Medium risk update detected. Continue? (y/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            return 0
        else
            return 1
        fi
    else
        log "ERROR" "DECISION" "🔴 HIGH RISK ($risk_score/100) - Update not recommended"
        echo -e "${RED}High risk update detected. Use --force to override.${NC}"
        return 1
    fi
}

# Enhanced compatibility checking with version matrix
check_enhanced_compatibility() {
    log "INFO" "COMPAT" "🔍 Running enhanced compatibility check..."
    
    local current_wlroots=$(get_wlroots_version)
    local required_wlroots=$(get_required_wlroots)
    
    # Version compatibility matrix
    declare -A compatibility_matrix=(
        ["0.19.0"]="3.5.0,3.5.1"
        ["0.19.1"]="3.5.1"
        ["0.19.2"]="3.5.1,3.6.0"
    )
    
    local axiom_version=$(grep -m1 "## \[" "$AXIOM_DIR/CHANGELOG.md" | sed 's/.*\[\(.*\)\].*/\1/')
    
    if [[ -n "${compatibility_matrix[$current_wlroots]:-}" ]]; then
        if [[ "${compatibility_matrix[$current_wlroots]}" == *"$axiom_version"* ]]; then
            log "INFO" "COMPAT" "✅ Version matrix confirms compatibility"
            return 0
        else
            log "WARN" "COMPAT" "⚠️  Version matrix indicates potential issues"
            return 1
        fi
    else
        log "WARN" "COMPAT" "⚠️  Unknown version combination in matrix"
        return 2
    fi
}

# Performance regression detection
detect_performance_regression() {
    log "INFO" "PERFORMANCE" "📊 Checking for performance regressions..."
    
    # Get baseline performance metrics
    local baseline_build_time=0
    local baseline_startup_time=0
    
    if [ -f "$PERFORMANCE_LOG" ]; then
        baseline_build_time=$(grep "Build completed" "$PERFORMANCE_LOG" | tail -5 | awk '{print $7}' | sed 's/s//' | awk '{sum+=$1} END {print sum/NR}' || echo "0")
        baseline_startup_time=$(grep "Startup time" "$PERFORMANCE_LOG" | tail -5 | awk '{print $7}' | sed 's/s//' | awk '{sum+=$1} END {print sum/NR}' || echo "0")
    fi
    
    # Measure current performance
    measure_build_time rebuild_axiom_silent
    measure_startup_time
    
    local current_build_time=$(cat /tmp/axiom_build_time.tmp 2>/dev/null || echo "0")
    local current_startup_time=$(cat /tmp/axiom_startup_time.tmp 2>/dev/null || echo "0")
    
    # Check for regressions (>20% slower)
    local build_regression=false
    local startup_regression=false
    
    if (( $(echo "$baseline_build_time > 0 && $current_build_time > $baseline_build_time * 1.2" | bc -l 2>/dev/null || echo "0") )); then
        build_regression=true
        log "WARN" "PERFORMANCE" "🐌 Build time regression detected: ${current_build_time}s vs ${baseline_build_time}s average"
    fi
    
    if (( $(echo "$baseline_startup_time > 0 && $current_startup_time > $baseline_startup_time * 1.2" | bc -l 2>/dev/null || echo "0") )); then
        startup_regression=true
        log "WARN" "PERFORMANCE" "🐌 Startup time regression detected: ${current_startup_time}s vs ${baseline_startup_time}s average"
    fi
    
    if [ "$build_regression" = "true" ] || [ "$startup_regression" = "true" ]; then
        return 1
    else
        log "INFO" "PERFORMANCE" "✅ No performance regressions detected"
        return 0
    fi
}

# Smart rollback system with automatic triggers
smart_rollback() {
    local rollback_reason=$1
    
    log "WARN" "ROLLBACK" "🔄 Initiating smart rollback due to: $rollback_reason"
    
    # Find most recent backup
    local latest_backup=$(ls -1t "$BACKUP_DIR"/axiom-backup-* 2>/dev/null | head -1)
    
    if [ -z "$latest_backup" ]; then
        log "ERROR" "ROLLBACK" "❌ No backups available for rollback"
        return 1
    fi
    
    log "INFO" "ROLLBACK" "📦 Rolling back to: $(basename "$latest_backup")"
    
    # Create a rollback point of current state
    local rollback_backup="$BACKUP_DIR/pre-rollback-$(date +%Y%m%d-%H%M%S)"
    cp -r "$AXIOM_DIR" "$rollback_backup"
    
    # Restore from backup
    rm -rf "$AXIOM_DIR"
    cp -r "$latest_backup" "$AXIOM_DIR"
    
    # Rebuild from restored state
    cd "$AXIOM_DIR"
    if rebuild_axiom_silent; then
        log "INFO" "ROLLBACK" "✅ Rollback completed successfully"
        return 0
    else
        log "ERROR" "ROLLBACK" "❌ Rollback build failed"
        return 1
    fi
}

# Silent rebuild for performance testing
rebuild_axiom_silent() {
    cd "$AXIOM_DIR"
    
    if [[ -d "build" ]]; then
        rm -rf build
    fi
    
    meson setup build >/dev/null 2>&1 && meson compile -C build >/dev/null 2>&1
}

# A/B testing framework for updates
ab_test_update() {
    log "INFO" "TESTING" "🧪 Running A/B test on update..."
    
    # Create A/B test backup
    local ab_backup="$BACKUP_DIR/ab-test-$(date +%Y%m%d-%H%M%S)"
    cp -r "$AXIOM_DIR" "$ab_backup"
    
    # Test current version (A)
    log "INFO" "TESTING" "Testing version A (current)..."
    measure_startup_time
    local version_a_startup=$(cat /tmp/axiom_startup_time.tmp)
    
    # Update and test new version (B)
    log "INFO" "TESTING" "Testing version B (updated)..."
    update_axiom_repo
    rebuild_axiom_silent
    measure_startup_time
    local version_b_startup=$(cat /tmp/axiom_startup_time.tmp)
    
    # Compare results
    log "INFO" "TESTING" "A/B Test Results:"
    log "INFO" "TESTING" "  Version A startup: ${version_a_startup}s"
    log "INFO" "TESTING" "  Version B startup: ${version_b_startup}s"
    
    # If B is significantly worse, recommend rollback
    if (( $(echo "$version_b_startup > $version_a_startup * 1.15" | bc -l 2>/dev/null || echo "0") )); then
        log "WARN" "TESTING" "⚠️  Version B shows performance degradation"
        return 1
    else
        log "INFO" "TESTING" "✅ Version B performance acceptable"
        return 0
    fi
}

# Machine learning-inspired pattern recognition
analyze_update_patterns() {
    log "INFO" "ML" "🤖 Analyzing historical update patterns..."
    
    if [ ! -f "$LOG_FILE" ]; then
        return 0
    fi
    
    # Pattern analysis
    local total_updates=$(grep -c "SUCCESS.*update completed" "$LOG_FILE" || echo "0")
    local failed_updates=$(grep -c "ERROR.*update.*failed" "$LOG_FILE" || echo "0")
    local rollbacks=$(grep -c "ROLLBACK" "$LOG_FILE" || echo "0")
    
    local success_rate=100
    if [ "$total_updates" -gt 0 ]; then
        success_rate=$(echo "scale=2; ($total_updates - $failed_updates) * 100 / $total_updates" | bc -l)
    fi
    
    log "INFO" "ML" "📊 Update Statistics:"
    log "INFO" "ML" "  Total updates: $total_updates"
    log "INFO" "ML" "  Failed updates: $failed_updates"
    log "INFO" "ML" "  Rollbacks: $rollbacks"
    log "INFO" "ML" "  Success rate: ${success_rate}%"
    
    # Predictive analysis based on patterns
    if (( $(echo "$success_rate < 80" | bc -l) )); then
        log "WARN" "ML" "⚠️  Low success rate detected - increasing risk assessment"
        return 1
    fi
    
    return 0
}

# Integration with original functions (preserved)
get_wlroots_version() {
    local current_version
    current_version=$(pacman -Q | grep wlroots | head -1 | awk '{print $2}' | cut -d'-' -f1)
    echo "$current_version"
}

get_required_wlroots() {
    if [[ -f "$AXIOM_DIR/meson.build" ]]; then
        grep "wlroots-" "$AXIOM_DIR/meson.build" | head -1 | sed -n "s/.*wlroots-\([0-9.]*\).*/\1/p"
    else
        echo "unknown"
    fi
}

update_axiom_repo() {
    cd "$AXIOM_DIR"
    git pull origin main
}

check_axiom_updates() {
    cd "$AXIOM_DIR"
    git fetch origin --quiet
    
    local ahead_count=$(git rev-list --count HEAD..origin/main 2>/dev/null || echo "0")
    
    if [[ "$ahead_count" -gt 0 ]]; then
        log "INFO" "UPDATE" "Found $ahead_count new commits available"
        return 0
    else
        log "INFO" "UPDATE" "Axiom is up to date (no new commits)"
        return 1
    fi
}

create_backup() {
    local backup_name="axiom-backup-$(date +%Y%m%d-%H%M%S)"
    local backup_path="$BACKUP_DIR/$backup_name"
    
    log "INFO" "BACKUP" "Creating backup: $backup_name"
    cp -r "$AXIOM_DIR" "$backup_path"
    log "INFO" "BACKUP" "✅ Backup created at: $backup_path"
    
    # Keep only last 10 backups (increased from 5)
    cd "$BACKUP_DIR"
    ls -1t axiom-backup-* | tail -n +11 | xargs -r rm -rf
}

# Enhanced main update process
main_smart_update() {
    log "INFO" "MAIN" "🚀 Starting Axiom Smart Update Process v2.0"
    
    # Analyze historical patterns
    analyze_update_patterns
    local pattern_risk=$?
    
    # Check for updates
    if ! check_axiom_updates; then
        log "INFO" "MAIN" "No updates available"
        return 0
    fi
    
    # Risk assessment
    local risk_score=$(assess_update_risk)
    if [ "$pattern_risk" -eq 1 ]; then
        risk_score=$((risk_score + 20))
        log "WARN" "RISK" "Increased risk due to historical patterns"
    fi
    
    # Make intelligent decision
    if ! make_update_decision "$risk_score" "${FORCE_UPDATE:-false}"; then
        log "INFO" "DECISION" "Update cancelled by intelligent decision system"
        return 1
    fi
    
    # Create backup
    create_backup
    
    # Enhanced compatibility check
    if ! check_enhanced_compatibility; then
        log "WARN" "COMPAT" "Compatibility concerns detected"
    fi
    
    # A/B test the update
    if ! ab_test_update; then
        log "WARN" "TESTING" "A/B test suggests issues with update"
        read -p "Continue despite A/B test concerns? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            smart_rollback "A/B test failure"
            return 1
        fi
    fi
    
    # Performance regression check
    if ! detect_performance_regression; then
        log "WARN" "PERFORMANCE" "Performance regression detected"
        smart_rollback "Performance regression"
        return 1
    fi
    
    log "INFO" "MAIN" "🎉 Smart update completed successfully!"
    return 0
}

# Load configuration
load_smart_config() {
    if [ -f "$CONFIG_FILE" ]; then
        source "$CONFIG_FILE"
    else
        mkdir -p "$(dirname "$CONFIG_FILE")"
        cat > "$CONFIG_FILE" << EOF
# Axiom Smart Updater v2 Configuration
ENABLE_RISK_ASSESSMENT=true
ENABLE_PERFORMANCE_MONITORING=true
ENABLE_AB_TESTING=true
ENABLE_ML_PATTERNS=true
MAX_RISK_SCORE=50
PERFORMANCE_REGRESSION_THRESHOLD=1.2
FORCE_UPDATE=false
EOF
        source "$CONFIG_FILE"
    fi
}

# Initialize
setup_smart_environment() {
    mkdir -p "$BACKUP_DIR" "$(dirname "$LOG_FILE")" "$(dirname "$PERFORMANCE_LOG")" "$(dirname "$RISK_ASSESSMENT_LOG")"
    load_smart_config
}

# Enhanced help
show_smart_help() {
    cat << EOF
${CYAN}Axiom Smart Updater v2.0 - Enhanced Intelligent Update System${NC}

${YELLOW}FEATURES:${NC}
  🧠 ML-based risk assessment
  📊 Performance regression detection  
  🧪 A/B testing framework
  🔄 Smart rollback system
  📈 Historical pattern analysis
  🎯 Enhanced compatibility matrix

${YELLOW}USAGE:${NC}
    $0 [OPTIONS]

${YELLOW}OPTIONS:${NC}
    --smart-update      Run full smart update process
    --risk-only         Perform risk assessment only
    --performance-test  Run performance tests only
    --ab-test          Run A/B test only
    --analyze-patterns  Analyze historical patterns
    --rollback         Smart rollback to previous version
    --help             Show this help

${YELLOW}LOGS:${NC}
    Main: $LOG_FILE
    Performance: $PERFORMANCE_LOG  
    Risk Assessment: $RISK_ASSESSMENT_LOG

${YELLOW}CONFIG:${NC}
    $CONFIG_FILE
EOF
}

# Command line parsing
case "${1:-}" in
    --smart-update)
        setup_smart_environment
        main_smart_update
        ;;
    --risk-only)
        setup_smart_environment
        risk_score=$(assess_update_risk)
        echo "Risk Score: $risk_score/100"
        ;;
    --performance-test)
        setup_smart_environment
        detect_performance_regression
        ;;
    --ab-test)
        setup_smart_environment
        ab_test_update
        ;;
    --analyze-patterns)
        setup_smart_environment
        analyze_update_patterns
        ;;
    --rollback)
        setup_smart_environment
        smart_rollback "Manual rollback requested"
        ;;
    --help|"")
        show_smart_help
        ;;
    *)
        echo -e "${RED}Unknown option: $1${NC}"
        show_smart_help
        exit 1
        ;;
esac
