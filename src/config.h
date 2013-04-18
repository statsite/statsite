#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <syslog.h>
#include <stdbool.h>

/**
 * Stores our configuration
 */
typedef struct {
    int tcp_port;
    int udp_port;
    char *bind_address;
    char *log_level;
    int syslog_log_level;
    double timer_eps;
    char *stream_cmd;
    int flush_interval;
    bool daemonize;
    char *pid_file;
    bool binary_stream;
    char *input_counter;
} statsite_config;

/**
 * Initializes the configuration from a filename.
 * Reads the file as an INI configuration, and sets up the
 * config object.
 * @arg filename The name of the file to read. NULL for defaults.
 * @arg config Output. The config object to initialize.
 * @return 0 on success, negative on error.
 */
int config_from_filename(char *filename, statsite_config *config);

/**
 * Validates the configuration
 * @arg config The config object to validate.
 * @return 0 on success, negative on error.
 */
int validate_config(statsite_config *config);

// Configuration validation methods
int sane_log_level(char *log_level, int *syslog_level);
int sane_timer_eps(double eps);
int sane_flush_interval(int intv);

/**
 * Joins two strings as part of a path,
 * and adds a separating slash if needed.
 * @param path Part one of the path
 * @param part2 The second part of the path
 * @return A new string, that uses a malloc()'d buffer.
 */
char* join_path(char *path, char *part2);

#endif
