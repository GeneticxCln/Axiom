#ifndef AXIOM_LOGGING_H
#define AXIOM_LOGGING_H

#include <stdbool.h>

typedef enum {
    AXIOM_LOG_DEBUG = 0,
    AXIOM_LOG_INFO = 1,
    AXIOM_LOG_WARN = 2,
    AXIOM_LOG_ERROR = 3
} axiom_log_level_t;

// Enhanced logging with component tags
void axiom_log_impl(axiom_log_level_t level, const char *format, ...);
void axiom_log_debug(const char *format, ...);
void axiom_log_info(const char *format, ...);
void axiom_log_warn(const char *format, ...);
void axiom_log_error(const char *format, ...);
void axiom_log(const char *level, const char *format, ...);

// Backward compatible macros (match existing code style)
// Support both single format and component-based logging
#define AXIOM_LOG_DEBUG(...) axiom_log_debug_variadic(__VA_ARGS__)
#define AXIOM_LOG_INFO(...) axiom_log_info_variadic(__VA_ARGS__)
#define AXIOM_LOG_WARN(...) axiom_log_warn_variadic(__VA_ARGS__)
#define AXIOM_LOG_ERROR(...) axiom_log_error_variadic(__VA_ARGS__)

// Enhanced macros with component tags (for new code)
#define AXIOM_LOG_DEBUG_COMPONENT(component, ...) \
    axiom_log_debug("[" component "] " __VA_ARGS__)

#define AXIOM_LOG_INFO_COMPONENT(component, ...) \
    axiom_log_info("[" component "] " __VA_ARGS__)

#define AXIOM_LOG_WARN_COMPONENT(component, ...) \
    axiom_log_warn("[" component "] " __VA_ARGS__)

#define AXIOM_LOG_ERROR_COMPONENT(component, ...) \
    axiom_log_error("[" component "] " __VA_ARGS__)

// Legacy compatibility
#define AXIOM_LOG(level, ...) \
    axiom_log(level, __VA_ARGS__)

// Variadic functions for component-based logging
void axiom_log_debug_variadic(const char* first, ...);
void axiom_log_info_variadic(const char* first, ...);
void axiom_log_warn_variadic(const char* first, ...);
void axiom_log_error_variadic(const char* first, ...);

// Configuration functions
void axiom_log_set_level(axiom_log_level_t level);
void axiom_log_set_enabled(bool enabled);
void axiom_log_set_file(const char *filename);
void axiom_log_cleanup(void);

#endif // AXIOM_LOGGING_H
