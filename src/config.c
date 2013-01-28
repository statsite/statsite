#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include "config.h"
#include "ini.h"

/**
 * Default statsite_config values. Should create
 * filters that are about 300KB initially, and suited
 * to grow quickly.
 */
static const statsite_config DEFAULT_CONFIG = {
    8125,               // TCP defaults to 8125
    8125,               // UDP on 8125
    "0.0.0.0",          // Listen on all IPv4 addresses
    "DEBUG",            // DEBUG level
    LOG_DEBUG,
    0.01,               // Default 1% error
    "cat",              // Pipe to cat
    10,                 // Flush every 10 seconds
    0,                  // Do not daemonize
    "/var/run/statsite.pid", // Default pidfile path
    0,                  // Do not use binary output by default
    NULL,               // Do not track number of messages received
};

/**
 * Attempts to convert a string to a boolean,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 1 on success, 0 on error.
 */
static bool value_to_bool(const char *val, bool *result) {
    #define VAL_MATCH(param) (strcasecmp(param, val) == 0)

    if (VAL_MATCH("true") || VAL_MATCH("yes") || VAL_MATCH("1")) {
        *result = true;
        return 0;
    } else if (VAL_MATCH("false") || VAL_MATCH("no") || VAL_MATCH("0")) {
        *result = false;
        return 0;
    }
    return 1;
}

/**
 * Attempts to convert a string to an integer,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 1 on success, 0 on error.
 */
static int value_to_int(const char *val, int *result) {
    long res = strtol(val, NULL, 10);
    if (res == 0 && errno == EINVAL) {
        return 0;
    }
    *result = res;
    return 1;
}

/**
 * Attempts to convert a string to a double,
 * and write the value out.
 * @arg val The string value
 * @arg result The destination for the result
 * @return 0 on success, -EINVAL on error.
 */
static int value_to_double(const char *val, double *result) {
    double res = strtod(val, NULL);
    if (res == 0) {
        return 0;
    }
    *result = res;
    return 0;
}

/**
 * Callback function to use with INI-H.
 * @arg user Opaque user value. We use the statsite_config pointer
 * @arg section The INI seciton
 * @arg name The config name
 * @arg value The config value
 * @return 1 on success.
 */
static int config_callback(void* user, const char* section, const char* name, const char* value) {
    // Ignore any non-statsite sections
    if (strcasecmp("statsite", section) != 0) {
        return 0;
    }

    // Cast the user handle
    statsite_config *config = (statsite_config*)user;

    // Switch on the config
    #define NAME_MATCH(param) (strcasecmp(param, name) == 0)

    // Handle the int cases
    if (NAME_MATCH("port")) {
        return value_to_int(value, &config->tcp_port);
    } else if (NAME_MATCH("tcp_port")) {
        return value_to_int(value, &config->tcp_port);
    } else if (NAME_MATCH("udp_port")) {
        return value_to_int(value, &config->udp_port);
    } else if (NAME_MATCH("flush_interval")) {
         return value_to_int(value, &config->flush_interval);
    } else if (NAME_MATCH("daemonize")) {
        return value_to_bool(value, &config->daemonize);
    } else if (NAME_MATCH("binary_stream")) {
        return value_to_bool(value, &config->binary_stream);

    // Handle the double cases
    } else if (NAME_MATCH("timer_eps")) {
        return value_to_double(value, &config->timer_eps);

    // Copy the string values
    } else if (NAME_MATCH("log_level")) {
        config->log_level = strdup(value);
    } else if (NAME_MATCH("stream_cmd")) {
        config->stream_cmd = strdup(value);
    } else if (NAME_MATCH("pid_file")) {
        config->pid_file = strdup(value);
    } else if (NAME_MATCH("input_counter")) {
        config->input_counter = strdup(value);
    } else if (NAME_MATCH("bind_address")) {
        config->bind_address = strdup(value);

    // Unknown parameter?
    } else {
        // Log it, but ignore
        syslog(LOG_NOTICE, "Unrecognized config parameter: %s", value);
    }

    // Success
    return 1;
}

/**
 * Initializes the configuration from a filename.
 * Reads the file as an INI configuration, and sets up the
 * config object.
 * @arg filename The name of the file to read. NULL for defaults.
 * @arg config Output. The config object to initialize.
 * @return 0 on success, negative on error.
 */
int config_from_filename(char *filename, statsite_config *config) {
    // Initialize to the default values
    memcpy(config, &DEFAULT_CONFIG, sizeof(statsite_config));

    // If there is no filename, return now
    if (filename == NULL)
        return 0;

    // Try to open the file
    int res = ini_parse(filename, config_callback, config);
    if (res == -1) {
        return -ENOENT;
    }

    return 0;
}

/**
 * Joins two strings as part of a path,
 * and adds a separating slash if needed.
 * @param path Part one of the path
 * @param part2 The second part of the path
 * @return A new string, that uses a malloc()'d buffer.
 */
char* join_path(char *path, char *part2) {
    // Check for the end slash
    int len = strlen(path);
    int has_end_slash = path[len-1] == '/';

    // Use the proper format string
    char *buf;
    int res;
    if (has_end_slash)
        res = asprintf(&buf, "%s%s", path, part2);
    else
        res = asprintf(&buf, "%s/%s", path, part2);
    assert(res != -1);

    // Return the new buffer
    return buf;
}

int sane_log_level(char *log_level, int *syslog_level) {
    #define LOG_MATCH(lvl) (strcasecmp(lvl, log_level) == 0)
    if (LOG_MATCH("DEBUG")) {
        *syslog_level = LOG_UPTO(LOG_DEBUG);
    } else if (LOG_MATCH("INFO")) {
        *syslog_level = LOG_UPTO(LOG_INFO);
    } else if (LOG_MATCH("WARN")) {
        *syslog_level = LOG_UPTO(LOG_WARNING);
    } else if (LOG_MATCH("ERROR")) {
        *syslog_level = LOG_UPTO(LOG_ERR);
    } else if (LOG_MATCH("CRITICAL")) {
        *syslog_level = LOG_UPTO(LOG_CRIT);
    } else {
        syslog(LOG_ERR, "Unknown log level!");
        return 1;
    }
    return 0;
}

int sane_timer_eps(double eps) {
    if (eps>= 0.5) {
        syslog(LOG_ERR,
               "Timer epsilon cannot be equal-to or greater than 0.5!");
        return 1;
    } else if (eps > 0.10) {
        syslog(LOG_WARNING, "Timer epsilon very high!");
    } else if (eps<= 0) {
        syslog(LOG_ERR,
               "Timer epsilon cannot less than or equal to 0!");
        return 1;
    }
    return 0;
}

int sane_flush_interval(int intv) {
    if (intv <= 0) {
        syslog(LOG_ERR, "Flush interval cannot be negative!");
        return 1;
    } else if (intv >= 600)  {
        syslog(LOG_WARNING,
               "Flushing set to be very infrequent! Increased risk of data loss.");
    }
    return 0;
}

/**
 * Validates the configuration
 * @arg config The config object to validate.
 * @return 0 on success.
 */
int validate_config(statsite_config *config) {
    int res = 0;

    res |= sane_log_level(config->log_level, &config->syslog_log_level);
    res |= sane_timer_eps(config->timer_eps);
    res |= sane_flush_interval(config->flush_interval);

    return res;
}

