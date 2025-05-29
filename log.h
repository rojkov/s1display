#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

/**
 * Log levels in ascending order of severity
 */
typedef enum {
    LOG_TRACE,  /**< Verbose debug information */
    LOG_DEBUG,  /**< Debug-level messages */
    LOG_INFO,   /**< General information */
    LOG_WARN,   /**< Warning messages */
    LOG_ERROR   /**< Error messages */
} log_level_t;

/**
 * Log a message with file and line information
 * @param level The log level
 * @param file The source file name
 * @param line The line number in the source file
 * @param fmt Printf-style format string
 * @param ... Variable arguments for the format string
 */
void log_log(int level, const char *file, int line, const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));

/**
 * Set the log level from a string
 * @param level_str One of: "trace", "debug", "info", "warn", "error"
 * @return 0 on success, -1 if the string is not a valid log level
 */
int log_set_level_by_string(const char *level_str);

/* Convenience macros for logging */
#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif /* LOG_H */
