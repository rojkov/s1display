#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int log_level = LOG_INFO;

static const char *get_level_str(int level) {
    switch (level) {
    case LOG_TRACE:
        return "TRACE";
    case LOG_DEBUG:
        return "DEBUG";
    case LOG_INFO:
        return "INFO";
    case LOG_WARN:
        return "WARN";
    case LOG_ERROR:
        return "ERROR";
    }

    return "";
}

void log_log(int level, const char *file, int line, const char *fmt,...) {
    va_list ap;

    if (level < log_level) {
        return;
    }

    printf("[%s] %s:%d ", get_level_str(level), file, line);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
}

static void log_set_level(int level) {
    log_level = level;
}

int log_set_level_by_string(const char *level_str) {
    if (strncmp(level_str, "trace", 5) == 0) {
        log_set_level(LOG_TRACE);
    } else if (strncmp(level_str, "debug", 5) == 0) {
        log_set_level(LOG_DEBUG);
    } else if (strncmp(level_str, "info", 4) == 0) {
        log_set_level(LOG_INFO);
    } else if (strncmp(level_str, "warn", 4) == 0) {
        log_set_level(LOG_WARN);
    } else if (strncmp(level_str, "error", 5) == 0) {
        log_set_level(LOG_ERROR);
    } else {
        return -1;
    }

    return 0;
}
