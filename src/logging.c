#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

typedef enum {
    AXIOM_LOG_DEBUG = 0,
    AXIOM_LOG_INFO = 1,
    AXIOM_LOG_WARN = 2,
    AXIOM_LOG_ERROR = 3
} axiom_log_level_t;

static axiom_log_level_t current_log_level = AXIOM_LOG_INFO;
static bool log_enabled = true;
static FILE *log_file = NULL;

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR"
};

void axiom_log_set_level(axiom_log_level_t level) {
    current_log_level = level;
}

void axiom_log_set_enabled(bool enabled) {
    log_enabled = enabled;
}

void axiom_log_set_file(const char *filename) {
    if (log_file && log_file != stderr) {
        fclose(log_file);
    }
    
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            log_file = stderr;
        }
    } else {
        log_file = stderr;
    }
}

void axiom_log_impl(axiom_log_level_t level, const char *format, ...) {
    if (!log_enabled || level < current_log_level) {
        return;
    }
    
    FILE *output = log_file ? log_file : stderr;
    
    // Get timestamp
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
    
    // Print level and timestamp
    fprintf(output, "[%s] [%s] ", timestamp, level_names[level]);
    
    // Print the actual message
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
    
    fprintf(output, "\n");
    fflush(output);
}

// Convenience functions
void axiom_log_debug(const char *format, ...) {
    if (!log_enabled || AXIOM_LOG_DEBUG < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[DEBUG] ");
    vfprintf(output, format, args);
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_info(const char *format, ...) {
    if (!log_enabled || AXIOM_LOG_INFO < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[INFO] ");
    vfprintf(output, format, args);
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_warn(const char *format, ...) {
    if (!log_enabled || AXIOM_LOG_WARN < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[WARN] ");
    vfprintf(output, format, args);
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_error(const char *format, ...) {
    if (!log_enabled || AXIOM_LOG_ERROR < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[ERROR] ");
    vfprintf(output, format, args);
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

// Legacy function for compatibility
void axiom_log(const char *level, const char *format, ...) {
    if (!log_enabled) {
        return;
    }
    
    FILE *output = log_file ? log_file : stderr;
    
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    
    fprintf(output, "[%02d:%02d:%02d] %s: ", 
           local->tm_hour, local->tm_min, local->tm_sec, level);
    
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);
    
    fprintf(output, "\n");
    fflush(output);
}

void axiom_log_cleanup(void) {
    if (log_file && log_file != stderr) {
        fclose(log_file);
        log_file = NULL;
    }
}
