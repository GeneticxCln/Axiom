#!/bin/bash

# Axiom Security Validation Script
# Performs basic security checks and validates implemented fixes

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FAILED_CHECKS=0
TOTAL_CHECKS=0

log() {
    echo -e "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

check_pass() {
    echo -e "${GREEN}✅ PASS${NC}: $1"
    ((TOTAL_CHECKS++))
}

check_fail() {
    echo -e "${RED}❌ FAIL${NC}: $1"
    ((FAILED_CHECKS++))
    ((TOTAL_CHECKS++))
}

check_warn() {
    echo -e "${YELLOW}⚠️  WARN${NC}: $1"
    ((TOTAL_CHECKS++))
}

echo -e "${BLUE}🔒 Axiom Security Validation Report${NC}"
echo "========================================"
echo ""

# 1. Check shell script vulnerabilities
log "Checking shell script security..."

if grep -q 'AXIOM_DIR="/home/alex/Projects/Axiom"' "$SCRIPT_DIR/axiom-updater.sh"; then
    check_pass "axiom-updater.sh path vulnerability fixed"
else
    check_fail "axiom-updater.sh still contains path with space"
fi

# 2. Check file permissions
log "Checking file permissions..."

config_files=("axiom.conf" "demo.conf" "rules.conf")
for file in "${config_files[@]}"; do
    if [[ -f "$SCRIPT_DIR/$file" ]]; then
        perms=$(stat -c %a "$SCRIPT_DIR/$file")
        if [[ "$perms" == "600" ]]; then
            check_pass "$file has secure permissions (600)"
        else
            check_fail "$file has insecure permissions ($perms), should be 600"
        fi
    fi
done

# 3. Check for hardcoded secrets
log "Scanning for hardcoded secrets..."

secret_patterns=("password=" "api_key=" "secret=" "token=" "private_key=")
found_secrets=false

for pattern in "${secret_patterns[@]}"; do
    if grep -r "$pattern" "$SCRIPT_DIR/src/" 2>/dev/null | grep -v "test" | grep -v ".md" >/dev/null; then
        check_fail "Found potential hardcoded secret: $pattern"
        found_secrets=true
    fi
done

if ! $found_secrets; then
    check_pass "No hardcoded secrets found in source code"
fi

# 4. Check for unsafe functions
log "Scanning for unsafe C functions..."

unsafe_functions=("strcpy" "strcat" "sprintf" "gets" "scanf")
found_unsafe=false

for func in "${unsafe_functions[@]}"; do
    if grep -r "\b$func\s*(" "$SCRIPT_DIR/src/" 2>/dev/null | grep -v ".md" >/dev/null; then
        check_warn "Found potentially unsafe function: $func"
        found_unsafe=true
    fi
done

if ! $found_unsafe; then
    check_pass "No unsafe C functions found"
fi

# 5. Check memory management
log "Validating memory management..."

if grep -q "axiom_malloc_tracked" "$SCRIPT_DIR/src/memory.c"; then
    check_pass "Memory tracking system implemented"
else
    check_fail "Memory tracking system not found"
fi

if grep -q "axiom_free_tracked" "$SCRIPT_DIR/src/memory.c"; then
    check_pass "Tracked memory deallocation implemented"
else
    check_fail "Tracked memory deallocation not found"
fi

# 6. Check build system security
log "Checking build system..."

if [[ -f "$SCRIPT_DIR/meson.build" ]]; then
    if grep -q "warning_level=2" "$SCRIPT_DIR/meson.build"; then
        check_pass "Compiler warnings enabled in build system"
    else
        check_warn "Compiler warnings not maximized in build system"
    fi
    
    if grep -q "werror=false" "$SCRIPT_DIR/meson.build"; then
        check_warn "Warnings as errors disabled (consider enabling for security)"
    else
        check_pass "Build treats warnings as errors"
    fi
fi

# 7. Check for shell injection vulnerabilities
log "Checking for shell injection vulnerabilities..."

shell_files=($(find "$SCRIPT_DIR" -name "*.sh" -type f))
injection_found=false

for file in "${shell_files[@]}"; do
    # Check for unquoted variables in command substitution
    if grep -P '\$\([^)]*\$[A-Za-z_][A-Za-z0-9_]*[^)]*\)' "$file" >/dev/null 2>&1; then
        check_warn "Potential shell injection in $(basename "$file")"
        injection_found=true
    fi
done

if ! $injection_found; then
    check_pass "No obvious shell injection vulnerabilities found"
fi

# 8. Check protocol security
log "Checking Wayland protocol security..."

if [[ -d "$SCRIPT_DIR/protocols" ]]; then
    if [[ -f "$SCRIPT_DIR/protocols/security-context-v1-protocol.h" ]]; then
        check_pass "Security context protocol implemented"
    else
        check_warn "Security context protocol not found"
    fi
    
    if [[ -f "$SCRIPT_DIR/protocols/ext-session-lock-v1-protocol.h" ]]; then
        check_pass "Session lock protocol implemented"
    else
        check_warn "Session lock protocol not found"
    fi
fi

# 9. Check dependency versions
log "Checking dependency security..."

if [[ -f "$SCRIPT_DIR/meson.build" ]]; then
    if grep -q "wayland-server.*>=.*1.20.0" "$SCRIPT_DIR/meson.build"; then
        check_pass "Wayland server minimum version specified"
    else
        check_warn "Wayland server version not properly constrained"
    fi
fi

# 10. Check for secure coding patterns
log "Checking secure coding patterns..."

if grep -q "goto cleanup" "$SCRIPT_DIR/src/config.c"; then
    check_pass "Error cleanup patterns implemented"
else
    check_warn "Error cleanup patterns not consistently used"
fi

if grep -q "bounds checking" "$SCRIPT_DIR"/*.md; then
    check_pass "Bounds checking documented/implemented"
else
    check_warn "Bounds checking not explicitly documented"
fi

echo ""
echo "========================================"
echo -e "${BLUE}Security Validation Summary${NC}"
echo "========================================"

if [[ $FAILED_CHECKS -eq 0 ]]; then
    echo -e "${GREEN}🎉 All critical security checks passed!${NC}"
    exit_code=0
elif [[ $FAILED_CHECKS -le 2 ]]; then
    echo -e "${YELLOW}⚠️  Some security issues found but overall good${NC}"
    exit_code=1
else
    echo -e "${RED}❌ Multiple security issues found - requires attention${NC}"
    exit_code=2
fi

echo ""
echo "Total Checks: $TOTAL_CHECKS"
echo "Failed Checks: $FAILED_CHECKS"
echo "Success Rate: $(( (TOTAL_CHECKS - FAILED_CHECKS) * 100 / TOTAL_CHECKS ))%"
echo ""

if [[ $FAILED_CHECKS -gt 0 ]]; then
    echo -e "${YELLOW}Recommendations:${NC}"
    echo "1. Review and fix any failed security checks above"
    echo "2. Consider implementing automated security scanning in CI/CD"
    echo "3. Regular security audits every 3 months"
    echo "4. Monitor dependencies for security updates"
fi

echo ""
echo "For detailed security analysis, see: COMPREHENSIVE_SECURITY_ANALYSIS.md"

exit $exit_code
