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
    log "${BLUE}🚀 Axiom Unified Updater${NC}"
    
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
    echo -e "${BLUE}Axiom Unified Updater - Intelligent update system${NC}"
    echo ""
    echo -e "${YELLOW}USAGE:${NC}"
    echo "    $0 [UPDATER_MODE] [OPTIONS]"
    echo ""
    echo -e "${YELLOW}UPDATER MODES:${NC}"
    echo "    --use-smart     Force use of smart updater v2"
    echo "    --use-legacy    Force use of legacy updater v1"
    echo "    --use-auto      Automatically select best updater (default)"
    echo ""
    echo -e "${YELLOW}LEGACY OPTIONS (converted automatically):${NC}"
    echo "    --check-only    Check for updates only"
    echo "    --force         Force update regardless of warnings"
    echo "    --rebuild       Rebuild without updating"
    echo "    --backup-only   Create backup only"
    echo "    --help          Show this help"
    echo ""
    echo -e "${YELLOW}SMART UPDATER OPTIONS:${NC}"
    echo "    --smart-update      Full intelligent update process"
    echo "    --risk-only         Risk assessment only"
    echo "    --performance-test  Performance testing only"
    echo "    --ab-test          A/B testing only"
    echo "    --analyze-patterns  Pattern analysis only"
    echo "    --rollback         Smart rollback"
    echo ""
    echo -e "${YELLOW}EXAMPLES:${NC}"
    echo "    $0                      # Auto-select updater and run"
    echo "    $0 --use-smart          # Force smart updater"
    echo "    $0 --use-legacy --force # Force legacy updater with force flag"
    echo "    $0 --check-only         # Check only (auto-converted)"
}

# Handle help requests
if [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]] || [[ $# -eq 0 && -t 0 ]]; then
    show_help
    exit 0
fi

# Run main function
main "$@"
