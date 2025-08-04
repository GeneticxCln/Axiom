#!/bin/bash

# Axiom Compositor Performance Profiling Script
# Monitors CPU, memory, and system performance during operation

LOG_DIR="/home/alex/axiom/performance_logs"
COMPOSITOR_PID=""
PROFILE_DURATION=60  # Default profiling duration in seconds

# Create logs directory
mkdir -p "$LOG_DIR"

echo "=== Axiom Compositor Performance Monitor ==="
echo "Log directory: $LOG_DIR"
echo "Profile duration: $PROFILE_DURATION seconds"
echo ""

# Function to find compositor PID
find_compositor_pid() {
    COMPOSITOR_PID=$(pgrep axiom)
    if [ -z "$COMPOSITOR_PID" ]; then
        echo "ERROR: Axiom compositor not running!"
        exit 1
    fi
    echo "Found Axiom compositor PID: $COMPOSITOR_PID"
}

# Function to monitor basic system stats
monitor_system_stats() {
    local duration=$1
    local output_file="$LOG_DIR/system_stats_$(date +%Y%m%d_%H%M%S).log"
    
    echo "=== System Statistics Monitoring ===" | tee "$output_file"
    echo "Duration: $duration seconds" | tee -a "$output_file"
    echo "Started at: $(date)" | tee -a "$output_file"
    echo "" | tee -a "$output_file"
    
    # System info
    echo "=== System Information ===" | tee -a "$output_file"
    uname -a | tee -a "$output_file"
    lscpu | grep "Model name\|CPU(s)\|Thread(s)" | tee -a "$output_file"
    free -h | tee -a "$output_file"
    echo "" | tee -a "$output_file"
    
    # Monitor for specified duration
    for i in $(seq 1 $duration); do
        echo "=== Sample $i ($(date)) ===" >> "$output_file"
        
        # CPU and memory usage for compositor
        ps -p $COMPOSITOR_PID -o pid,ppid,pcpu,pmem,rss,vsz,comm >> "$output_file" 2>/dev/null
        
        # Overall system load
        echo "Load average:" >> "$output_file"
        uptime >> "$output_file"
        
        # Memory usage
        echo "Memory usage:" >> "$output_file"
        free -m >> "$output_file"
        
        # GPU memory if available
        if command -v nvidia-smi >/dev/null 2>&1; then
            echo "GPU memory:" >> "$output_file"
            nvidia-smi --query-gpu=memory.used,memory.total --format=csv,noheader,nounits >> "$output_file" 2>/dev/null
        fi
        
        echo "" >> "$output_file"
        sleep 1
    done
    
    echo "System stats monitoring complete. Log: $output_file"
}

# Function to profile with perf (if available)
profile_with_perf() {
    local duration=$1
    local output_file="$LOG_DIR/perf_profile_$(date +%Y%m%d_%H%M%S).data"
    
    if ! command -v perf >/dev/null 2>&1; then
        echo "perf not available, skipping CPU profiling"
        return
    fi
    
    echo "=== CPU Profiling with perf ==="
    echo "Profiling PID $COMPOSITOR_PID for $duration seconds..."
    
    # Record performance data
    perf record -p $COMPOSITOR_PID -g -o "$output_file" sleep $duration 2>/dev/null
    
    if [ -f "$output_file" ]; then
        # Generate report
        local report_file="$LOG_DIR/perf_report_$(date +%Y%m%d_%H%M%S).txt"
        perf report -i "$output_file" --stdio > "$report_file" 2>/dev/null
        echo "CPU profile complete. Data: $output_file, Report: $report_file"
    else
        echo "perf recording failed or requires privileges"
    fi
}

# Function to monitor memory usage patterns
monitor_memory_patterns() {
    local duration=$1
    local output_file="$LOG_DIR/memory_pattern_$(date +%Y%m%d_%H%M%S).log"
    
    echo "=== Memory Pattern Analysis ===" | tee "$output_file"
    echo "Monitoring PID $COMPOSITOR_PID for $duration seconds" | tee -a "$output_file"
    echo "" | tee -a "$output_file"
    
    for i in $(seq 1 $duration); do
        if [ $((i % 5)) -eq 0 ]; then  # Every 5 seconds
            echo "=== Memory Sample $i ===" >> "$output_file"
            
            # Detailed memory info for compositor
            if [ -f "/proc/$COMPOSITOR_PID/status" ]; then
                grep -E "VmPeak|VmSize|VmRSS|VmData|VmStk|VmExe|VmLib" "/proc/$COMPOSITOR_PID/status" >> "$output_file"
            fi
            
            # Memory maps (first few lines)
            if [ -f "/proc/$COMPOSITOR_PID/smaps" ]; then
                echo "Memory segments (sample):" >> "$output_file"
                head -20 "/proc/$COMPOSITOR_PID/smaps" >> "$output_file"
            fi
            
            echo "" >> "$output_file"
        fi
        sleep 1
    done
    
    echo "Memory pattern analysis complete. Log: $output_file"
}

# Function to stress test with multiple clients
stress_test_with_clients() {
    local num_clients=${1:-5}
    local test_duration=${2:-30}
    
    echo "=== Stress Test with Multiple Clients ==="
    echo "Launching $num_clients test clients for $test_duration seconds..."
    
    # Launch multiple test clients
    local client_pids=()
    for i in $(seq 1 $num_clients); do
        WAYLAND_DISPLAY=wayland-1 weston-simple-egl &
        client_pids+=($!)
        sleep 0.5
    done
    
    echo "Launched ${#client_pids[@]} clients: ${client_pids[*]}"
    
    # Monitor during stress test
    local stress_log="$LOG_DIR/stress_test_$(date +%Y%m%d_%H%M%S).log"
    echo "=== Stress Test Results ===" | tee "$stress_log"
    echo "Clients: $num_clients, Duration: $test_duration seconds" | tee -a "$stress_log"
    echo "Started at: $(date)" | tee -a "$stress_log"
    echo "" | tee -a "$stress_log"
    
    # Monitor for test duration
    for i in $(seq 1 $test_duration); do
        if [ $((i % 5)) -eq 0 ]; then
            echo "=== Stress Sample $i ===" >> "$stress_log"
            ps -p $COMPOSITOR_PID -o pid,pcpu,pmem,rss,vsz >> "$stress_log" 2>/dev/null
            echo "Active clients: $(pgrep -l weston | wc -l)" >> "$stress_log"
            uptime >> "$stress_log"
            echo "" >> "$stress_log"
        fi
        sleep 1
    done
    
    # Clean up test clients
    for pid in "${client_pids[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    
    echo "Stress test complete. Log: $stress_log"
}

# Function to analyze logs for issues
analyze_performance_logs() {
    echo "=== Performance Log Analysis ==="
    
    if [ ! -d "$LOG_DIR" ] || [ -z "$(ls -A $LOG_DIR 2>/dev/null)" ]; then
        echo "No performance logs found"
        return
    fi
    
    local analysis_file="$LOG_DIR/analysis_$(date +%Y%m%d_%H%M%S).txt"
    echo "=== Performance Analysis Report ===" > "$analysis_file"
    echo "Generated at: $(date)" >> "$analysis_file"
    echo "" >> "$analysis_file"
    
    # Find peak memory usage
    if ls "$LOG_DIR"/system_stats_*.log >/dev/null 2>&1; then
        echo "=== Peak Memory Usage ===" >> "$analysis_file"
        grep -h "axiom" "$LOG_DIR"/system_stats_*.log | awk '{print $6}' | sort -n | tail -5 >> "$analysis_file"
        echo "" >> "$analysis_file"
    fi
    
    # Find peak CPU usage
    if ls "$LOG_DIR"/system_stats_*.log >/dev/null 2>&1; then
        echo "=== Peak CPU Usage ===" >> "$analysis_file"
        grep -h "axiom" "$LOG_DIR"/system_stats_*.log | awk '{print $3}' | sort -n | tail -5 >> "$analysis_file"
        echo "" >> "$analysis_file"
    fi
    
    echo "Performance analysis complete. Report: $analysis_file"
}

# Main menu
main_menu() {
    echo "Select profiling option:"
    echo "1) Quick System Stats (60s)"
    echo "2) CPU Profiling with perf (30s)"
    echo "3) Memory Pattern Analysis (60s)" 
    echo "4) Stress Test with Multiple Clients"
    echo "5) Full Performance Suite"
    echo "6) Analyze Existing Logs"
    echo "7) Custom Duration Test"
    echo ""
    
    read -p "Enter choice (1-7): " choice
    
    case $choice in
        1) monitor_system_stats 60 ;;
        2) profile_with_perf 30 ;;
        3) monitor_memory_patterns 60 ;;
        4) stress_test_with_clients 5 30 ;;
        5) 
            echo "Running full performance suite..."
            monitor_system_stats 60 &
            profile_with_perf 30 &
            monitor_memory_patterns 60 &
            wait
            stress_test_with_clients 3 30
            analyze_performance_logs
            ;;
        6) analyze_performance_logs ;;
        7) 
            read -p "Enter duration in seconds: " custom_duration
            monitor_system_stats "$custom_duration"
            ;;
        *) echo "Invalid choice" ;;
    esac
}

# Main execution
find_compositor_pid
main_menu
