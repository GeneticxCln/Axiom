#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void axiom_log(const char *level, const char *format, ...) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);
    
    printf("[%02d:%02d:%02d] %s: ", 
           local->tm_hour, local->tm_min, local->tm_sec, level);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}
