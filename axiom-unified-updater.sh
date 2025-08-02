#!/bin/bash

# Axiom Unified Updater - Intelligent update system with fallback support

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
    local required_commands=("bc" "git" "pacman" "meson")
    
    for cmd in "${required_commands[@]}"; do
        if ! command -v "$cmd" >/dev/null 2>&1; then
            log "${YELLOW}⚠️  Missing command: $cmd - falling back to legacy updater${NC}"
            return 1
        fi
    done
    
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
                echo "$SMART_UPDATER"
            else
                echo "$LEGACY_UPDATER"
            fi
            ;;
        *)
            log "${RED}❌ Unknown updater mode: $mode${NC}"
            exit 1
            ;;
    esac
}

# Convert arguments
convert_arguments() {
    local selected_updater="$1"
    shift
    local args=("$@")
    
    if [[ "$selected_updater" == *"smart-updater"* ]]; then
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
                    "$LEGACY_UPDATER" --backup-only
                    exit $?
                    ;;
                "--help")
                    smart_args+=("--help")
                    ;;
                *)
                    smart_args+=("--smart-update")
                    ;;
            esac
        done
        
        if [ ${#smart_args[@]} -eq 0 ]; then
            smart_args+=("--smart-update")
        fi
        
        echo "${smart_args[@]}"
    else
        echo "${args[@]}"
    fi
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
    echo -e "${YELLOW}OPTIONS:${NC}"
    echo "    --check-only    Check for updates only"
    echo "    --force         Force update regardless of warnings"
    echo "    --rebuild       Rebuild without updating"
    echo "    --backup-only   Create backup only"
    echo "    --help          Show this help"
}

# Main execution
main() {
    log "${BLUE}🚀 Axiom Unified Updater${NC}"
    
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
    
    local selected_updater=$(select_updater "$updater_mode")
    local final_args=$(convert_arguments "$selected_updater" "${filtered_args[@]}")
    
    log "${BLUE}Executing: $(basename "$selected_updater") $final_args${NC}"
    "$selected_updater" $final_args
}

# Handle help requests
if [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]] || [[ $# -eq 0 && -t 0 ]]; then
    show_help
    exit 0
fi

# Run main function
main "$@"
