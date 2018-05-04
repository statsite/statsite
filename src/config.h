#ifndef CONFIG_H
#define CONFIG_H
#include <stdint.h>
#include <syslog.h>
#include <stdbool.h>
#include "radix.h"

typedef enum {
    UNKNOWN,
    KEY_VAL,
    GAUGE,
    COUNTER,
    TIMER,
    SET,
    GAUGE_DELTA
} metric_type;

#define METRIC_TYPES 7

typedef struct included_metrics_config {
    bool count;
    bool mean;
    bool stdev;
    bool sum;
    bool sum_sq;
    bool lower;
    bool upper;
    bool rate;
    bool median;
    bool sample_rate;
} included_metrics_config;

// Represents the configuration of a histogram
typedef struct histogram_config {
    char *prefix;
    double min_val;
    double max_val;
    double bin_width;
    int num_bins;
    struct histogram_config *next;
    char parts;
} histogram_config;


/**
 * Stores our configuration
 */
typedef struct {
    int tcp_port;
    int udp_port;
    int udp_rcvbuf;
    char *bind_address;
    bool parse_stdin;
    char *log_level;
    int syslog_log_level;
    char *log_facility;
    int syslog_log_facility;
    double timer_eps;
    char *stream_cmd;
    int flush_interval;
    bool daemonize;
    bool aligned_flush;
    char *pid_file;
    bool binary_stream;
    char *input_counter;
    histogram_config *hist_configs;
    radix_tree *histograms;
    double set_eps;
    unsigned char set_precision;
    bool use_type_prefix;
    char* global_prefix;
    char* prefixes[METRIC_TYPES];
    char* prefixes_final[METRIC_TYPES];
    bool extended_counters;
    bool legacy_extended_counters;
    included_metrics_config timers_config;
    bool prefix_binary_stream;
    int num_quantiles;
    double* quantiles;
} statsite_config;

/**
 * Allocates memory for a new config structure
 * @return a pointer to a new config structure on success.
 */
statsite_config* alloc_config();

/**
 * Frees memory associated with a previously allocated config structure
 * @arg config The config object to free.
 */
void free_config(statsite_config* config);

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
 * Gets a final prefix string for each message type
 * @arg config Output. The config object to prepare strings.
 */

int prepare_prefixes(statsite_config *config);

/**
 * Validates the configuration
 * @arg config The config object to validate.
 * @return 0 on success, negative on error.
 */
int validate_config(statsite_config *config);

// Configuration validation methods
int sane_log_level(char *log_level, int *syslog_level);
int sane_log_facility(char *log_facil, int *syslog_facility);
int sane_timer_eps(double eps);
int sane_flush_interval(int intv);
int sane_histograms(histogram_config *config);
int sane_set_precision(double eps, unsigned char *precision);
int sane_quantiles(int num_quantiles, double quantiles[]);

/**
 * Joins two strings as part of a path,
 * and adds a separating slash if needed.
 * @param path Part one of the path
 * @param part2 The second part of the path
 * @return A new string, that uses a malloc()'d buffer.
 */
char* join_path(char *path, char *part2);

/**
 * Builds the radix tree for prefix matching
 * @return 0 on success
 */
int build_prefix_tree(statsite_config *config);

#endif
