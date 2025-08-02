#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

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

// Variadic helper macros to support both single format and component-based logging
#define GET_ARG_COUNT(...) GET_ARG_COUNT_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define GET_ARG_COUNT_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N

// Get first two arguments for component detection
#define GET_FIRST(first, ...) first
#define GET_SECOND(first, second, ...) second

// Check if first argument is a component name (simple heuristic: single word without format specifiers)
static bool is_component_name(const char* str) {
    // Simple check: component names are typically single words without % characters
    if (!str) return false;
    
    // If it contains %, it's likely a format string
    if (strchr(str, '%') != NULL) return false;
    
    // If it's all uppercase or mixed case without spaces, likely a component
    bool has_space = strchr(str, ' ') != NULL;
    return !has_space && strlen(str) < 32;  // Reasonable component name limit
}

// Variadic wrapper functions
void axiom_log_debug_variadic(const char* first, ...) {
    if (!log_enabled || AXIOM_LOG_DEBUG < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, first);
    
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[DEBUG] ");
    
    if (is_component_name(first)) {
        // Component-based logging: AXIOM_LOG_DEBUG("COMPONENT", "format", ...)
        const char* format = va_arg(args, const char*);
        fprintf(output, "[%s] ", first);
        vfprintf(output, format, args);
    } else {
        // Regular logging: AXIOM_LOG_DEBUG("format", ...)
        vfprintf(output, first, args);
    }
    
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_info_variadic(const char* first, ...) {
    if (!log_enabled || AXIOM_LOG_INFO < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, first);
    
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[INFO] ");
    
    if (is_component_name(first)) {
        // Component-based logging: AXIOM_LOG_INFO("COMPONENT", "format", ...)
        const char* format = va_arg(args, const char*);
        fprintf(output, "[%s] ", first);
        vfprintf(output, format, args);
    } else {
        // Regular logging: AXIOM_LOG_INFO("format", ...)
        vfprintf(output, first, args);
    }
    
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_warn_variadic(const char* first, ...) {
    if (!log_enabled || AXIOM_LOG_WARN < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, first);
    
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[WARN] ");
    
    if (is_component_name(first)) {
        // Component-based logging: AXIOM_LOG_WARN("COMPONENT", "format", ...)
        const char* format = va_arg(args, const char*);
        fprintf(output, "[%s] ", first);
        vfprintf(output, format, args);
    } else {
        // Regular logging: AXIOM_LOG_WARN("format", ...)
        vfprintf(output, first, args);
    }
    
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}

void axiom_log_error_variadic(const char* first, ...) {
    if (!log_enabled || AXIOM_LOG_ERROR < current_log_level) {
        return;
    }
    
    va_list args;
    va_start(args, first);
    
    FILE *output = log_file ? log_file : stderr;
    fprintf(output, "[ERROR] ");
    
    if (is_component_name(first)) {
        // Component-based logging: AXIOM_LOG_ERROR("COMPONENT", "format", ...)
        const char* format = va_arg(args, const char*);
        fprintf(output, "[%s] ", first);
        vfprintf(output, format, args);
    } else {
        // Regular logging: AXIOM_LOG_ERROR("format", ...)
        vfprintf(output, first, args);
    }
    
    fprintf(output, "\n");
    fflush(output);
    va_end(args);
}
