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
#include "hll.h"

/**
 * Allow more succint if blocks
 */
#define NAME_MATCH(param) (strcasecmp(param, name) == 0)

/**
 * Static pointer used for while we are still parsing the configs for
 * a histogram or a sink.
 *
 * This is somewhat of a hack but its used only once. The alternative
 * it to use a pure kv reader into memory and then re-parse the kv
 * tree, but that was a larger yak-shave.
 */
static char* histogram_section;
static histogram_config *histogram_in_progress;

static char* sink_section;
static sink_config *sink_in_progress;

/**
 * Default statsite_config values. Should create
 * filters that are about 300KB initially, and suited
 * to grow quickly.
 */
static double default_quantiles[] = {0.5, 0.95, 0.99};
static const statsite_config DEFAULT_CONFIG = {
    8125,               // TCP defaults to 8125
    8125,               // UDP on 8125
    "::",               // Listen on all addresses
    false,              // Do not parse stdin by default
    "DEBUG",            // DEBUG level
    LOG_DEBUG,
    "local0",           // local0 logging facility
    LOG_LOCAL0,         // Syslog logging facility
    0.01,               // Default 1% error
    10,                 // Flush every 10 seconds
    0,                  // Do not daemonize
    "/var/run/statsite.pid", // Default pidfile path
    NULL,               // Do not track number of messages received
    NULL,               // No histograms by default
    NULL,               // No sinks are built
    NULL,
    0.02,               // 2% goal uses precision 12
    12,                 // Set precision 12, 1.6% variance
    true,               // Use type prefixes by default
    "",                 // Global prefix
    {"", "kv.", "gauges.", "counts.", "timers.", "sets.", ""},
    {},
    false,              // Extended counts off by default
    false,              // Do not prefix binary stream by default
                        // Number of quantiles
    sizeof(default_quantiles) / sizeof(double),
    default_quantiles,  // Quantiles
};

static const sink_config_stream DEFAULT_SINK = {
    .super = { .type = SINK_TYPE_STREAM,
               .name = "default",
               .next = NULL
    },
    .binary_stream = false,
    .stream_cmd = "cat"
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
        return 1;
    } else if (VAL_MATCH("false") || VAL_MATCH("no") || VAL_MATCH("0")) {
        *result = false;
        return 1;
    }
    return 0;
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
 * @return 1 on success, -EINVAL on error.
 */
static int value_to_double(const char *val, double *result) {
    return sscanf(val, "%lf", result);
}

/**
 * Attempts to convert a string to an array out doubles,
 * assuming a comma and space separated list.
 * @arg val The string value
 * @arg result The destination for the result
 * @arg count The destination for the count
 * @return 1 on success, 0 on error.
 */
static int value_to_list_of_doubles(const char *val, double **result, int *count) {
    int scanned;
    double quantile;

    *count = 0;
    *result = NULL;
    while (sscanf(val, "%lf%n", &quantile, &scanned) == 1) {
        val += scanned;
        *count += 1;
        *result = realloc(*result, *count * sizeof(double));
        (*result)[*count - 1] = quantile;
        if (sscanf(val, " , %n", &scanned) == 0) {
            val += scanned;
        }
    }
    sscanf(val, " %n", &scanned);

    return val[scanned] == '\0';
}

/**
* Attempts to convert a string log facility
* to an actual syslog log facility,
* @arg val The string value
* @arg result The destination for the result
* @return 1 on success, 0 on error.
*/
static int name_to_facility(const char *val, int *result) {
    int log_facility = LOG_LOCAL0;
    if (VAL_MATCH("local0")) {
        log_facility = LOG_LOCAL0;
    } else if (VAL_MATCH("local1")) {
        log_facility = LOG_LOCAL1;
    } else if (VAL_MATCH("local2")) {
        log_facility = LOG_LOCAL2;
    } else if (VAL_MATCH("local3")) {
        log_facility = LOG_LOCAL3;
    } else if (VAL_MATCH("local4")) {
        log_facility = LOG_LOCAL4;
    } else if (VAL_MATCH("local5")) {
        log_facility = LOG_LOCAL5;
    } else if (VAL_MATCH("local6")) {
        log_facility = LOG_LOCAL6;
    } else if (VAL_MATCH("local7")) {
        log_facility = LOG_LOCAL7;
    } else if (VAL_MATCH("user")) {
        log_facility = LOG_LOCAL7;
    } else if (VAL_MATCH("daemon")) {
        log_facility = LOG_LOCAL7;
    } else {
        log_facility = LOG_LOCAL0;
    }

    if (errno == EINVAL) {
        return 0;
    }

    *result = log_facility;
    return 1;
}

/**
 * Commit any in-progress sinks, either when we switched sections
 * or when the configuration file has ended.
 */
static void sink_commit(statsite_config *config) {
    /* Todo: validate something here */

    if (sink_in_progress) {
        sink_config* last = config->sink_configs;
        if (last)
            sink_in_progress->next = last;
        config->sink_configs = sink_in_progress;
    }

    /* Cleanup */
    sink_in_progress = NULL;
    if (sink_section) {
        free(sink_section);
        sink_section = NULL;
    }
    return;
}

/**
 * Callback function to use with INIH for parsing sink configurations.
 * The sink type is currently encoded into the section name to
 * simplify instantiating the correct configuration type.
 */
static int sink_callback(void* user, const char* section, const char* name, const char* value) {

    statsite_config *all_config = (statsite_config*)user;

    /* The sink section does not match - lets commit since we're on to a new one now */
    if (sink_in_progress && strcasecmp(sink_section, section)) {
        sink_commit(all_config);
    }

    /* Nothing in progress? Create it! */
    if (!sink_in_progress) {
        if (!sink_section)
            sink_section = strdup(section);

        char* section_to_tokenize = strdup(section);
        char* tok = NULL;
        char* header = strtok_r(section_to_tokenize, "_", &tok);
        char* type = strtok_r(NULL, "_", &tok);
        char* name = strtok_r(NULL, "_", &tok);

        if (header == NULL || type == NULL || name == NULL) {
            free(section_to_tokenize);
            syslog(LOG_WARNING, "Sink section %s is not of the form \"sink_[type]_[name]\"", section);
            return 0;
        }
        /* Match various sink types to find their type */
        if (strcasecmp(type, "stream") == 0) {
            sink_config_stream* config = calloc(1, sizeof(sink_config_stream));
            sink_in_progress = (sink_config*)config;
            config->super.type = SINK_TYPE_STREAM;
            config->super.name = strdup(name);
            config->stream_cmd = DEFAULT_SINK.stream_cmd;
        } else if (strcasecmp(type, "http") == 0) {
            sink_config_http* config = calloc(1, sizeof(sink_config_http));
            sink_in_progress = (sink_config*)config;
            config->super.type = SINK_TYPE_HTTP;
            config->super.name = strdup(name);
        } else {
            free(section_to_tokenize);
            /* Unknown sink type - abort! */
            syslog(LOG_WARNING, "Unknown sink type: %s for sink: %s", type, name);
            return 0;
        }
        free(section_to_tokenize);
    }

    switch(sink_in_progress->type) {
    case SINK_TYPE_STREAM:
    {
        sink_config_stream* config = (sink_config_stream*)sink_in_progress;
        if (NAME_MATCH("binary")) {
            return value_to_bool(value, &config->binary_stream);
        } else if (NAME_MATCH("command")) {
            config->stream_cmd = strdup(value);
        } else {
            syslog(LOG_NOTICE, "Unrecognized stream sink parameter: %s", name);
            return 0;
        }
        break;
    }
    case SINK_TYPE_HTTP:
    {
        sink_config_http* config = (sink_config_http*)sink_in_progress;
        if (NAME_MATCH("url")) {
            config->post_url = strdup(value);
        } else {
            /* Attempt to locate keys
             * of the form param_PNAME */
            char* param_tok = strdup(name);
            char* tok = NULL;
            char* header = strtok_r(param_tok, "_", &tok);
            char* param = strtok_r(NULL, "_", &tok);
            if (header == NULL || param == NULL || strcasecmp("param", header) != 0) {
                syslog(LOG_NOTICE, "Unrecognized http sink parameters: %s: %s", name, header);
                free(param_tok);
                return 0;
            }
            kv_config* last_kv = config->params;
            config->params = calloc(1, sizeof(kv_config));
            config->params->k = strdup(param);
            config->params->v = strdup(value);
            config->params->next = last_kv;
            free(param_tok);
        }
        break;
    }
    default:
        syslog(LOG_WARNING, "Grevious state problem");
        return 0;
    }

    return 1;
}


/**
 * Callback function to use with INIH for parsing histogram configs
 * @arg user Opaque value. Actually a statsite_config pointer
 * @arg name The config name
 * @value = The config value
 * @return 1 on success
 */
static int histogram_callback(void* user, const char* section, const char* name, const char* value) {
    // Make sure we don't change sections with an unfinished config
    if (histogram_in_progress && strcasecmp(histogram_section, section)) {
        syslog(LOG_WARNING, "Unfinished configuration for section: %s", histogram_section);
        return 0;
    }

    // Ensure we have something in progress
    if (!histogram_in_progress) {
        histogram_in_progress = calloc(1, sizeof(histogram_config));
        histogram_section = strdup(section);
    }

    // Cast the user handle
    statsite_config *config = (statsite_config*)user;

    int res = 1;
    if (NAME_MATCH("prefix")) {
        histogram_in_progress->parts |= 1;
        histogram_in_progress->prefix = strdup(value);

    } else if (NAME_MATCH("min")) {
        histogram_in_progress->parts |= 1 << 1;
        res = value_to_double(value, &histogram_in_progress->min_val);

    } else if (NAME_MATCH("max")) {
        histogram_in_progress->parts |= 1 << 2;
        res = value_to_double(value, &histogram_in_progress->max_val);

    } else if (NAME_MATCH("width")) {
        histogram_in_progress->parts |= 1 << 3;
        res = value_to_double(value, &histogram_in_progress->bin_width);

    } else {
        syslog(LOG_NOTICE, "Unrecognized histogram config parameter: %s", value);
    }

    // Check if this config is done, and push into the list of configs
    if (histogram_in_progress->parts == 15) {
        histogram_in_progress->next = config->hist_configs;
        config->hist_configs = histogram_in_progress;
        histogram_in_progress = NULL;
        free(histogram_section);
        histogram_section = NULL;
    }
    return res;
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
    // Specially handle histogram sections
    if (strncasecmp("histogram", section, 9) == 0) {
        return histogram_callback(user, section, name, value);
    }

    if (strncasecmp("sink", section, 4) == 0) {
        return sink_callback(user, section, name, value);
    }

    // Ignore any non-statsite sections
    if (strcasecmp("statsite", section) != 0) {
        syslog(LOG_NOTICE, "Unknown values in section ignored: %s", section);
        return 0;
    }

    // Cast the user handle
    statsite_config *config = (statsite_config*)user;

    // Handle the int cases
    if (NAME_MATCH("port")) {
        return value_to_int(value, &config->tcp_port);
    } else if (NAME_MATCH("tcp_port")) {
        return value_to_int(value, &config->tcp_port);
    } else if (NAME_MATCH("udp_port")) {
        return value_to_int(value, &config->udp_port);
    } else if (NAME_MATCH("flush_interval")) {
         return value_to_int(value, &config->flush_interval);
    } else if (NAME_MATCH("parse_stdin")) {
        return value_to_bool(value, &config->parse_stdin);
    } else if (NAME_MATCH("daemonize")) {
        return value_to_bool(value, &config->daemonize);
    } else if (NAME_MATCH("use_type_prefix")) {
        return value_to_bool(value, &config->use_type_prefix);
    } else if (NAME_MATCH("extended_counters")) {
        return value_to_bool(value, &config->extended_counters);
    } else if (NAME_MATCH("prefix_binary_stream")) {
        return value_to_bool(value, &config->prefix_binary_stream);

    // Handle the double cases
    } else if (NAME_MATCH("timer_eps")) {
        return value_to_double(value, &config->timer_eps);
    } else if (NAME_MATCH("set_eps")) {
        return value_to_double(value, &config->set_eps);

    // Handle quantiles as a comma-separated list of doubles
    } else if (NAME_MATCH("quantiles")) {
        return value_to_list_of_doubles(value, &config->quantiles, &config->num_quantiles);

    // Copy the string values
    } else if (NAME_MATCH("log_level")) {
        config->log_level = strdup(value);
    } else if (NAME_MATCH("log_facility")) {
        config->log_facility = strdup(value);
    } else if (NAME_MATCH("pid_file")) {
        config->pid_file = strdup(value);
    } else if (NAME_MATCH("input_counter")) {
        config->input_counter = strdup(value);
    } else if (NAME_MATCH("bind_address")) {
        config->bind_address = strdup(value);
    } else if (NAME_MATCH("global_prefix")) {
        config->global_prefix = strdup(value);
    } else if (NAME_MATCH("counts_prefix")) {
        config->prefixes[COUNTER] = strdup(value);
    } else if (NAME_MATCH("gauges_prefix")) {
        config->prefixes[GAUGE] = strdup(value);
    } else if (NAME_MATCH("timers_prefix")) {
        config->prefixes[TIMER] = strdup(value);
    } else if (NAME_MATCH("sets_prefix")) {
        config->prefixes[SET] = strdup(value);
    } else if (NAME_MATCH("kv_prefix")) {
        config->prefixes[KEY_VAL] = strdup(value);

    // Copy the multi-case variables
    } else if (NAME_MATCH("log_facility")) {
        return name_to_facility(value, &config->syslog_log_facility);

    // Unknown parameter?
    } else {
        // Log it, but ignore
        syslog(LOG_NOTICE, "Unrecognized config parameter: %s", name);
    }

    // Success
    return 1;
}

/**
 * Gets a final prefix string for each message type
 * @arg config Output. The config object to prepare strings.
 */

int prepare_prefixes(statsite_config *config)
{
    int res;
    char *final_prefix, *type_prefix = "";
    for (int i=0; i < METRIC_TYPES; i++) {
        // Get the type prefix
        if (config->use_type_prefix) {
            type_prefix = config->prefixes[i];
        }

        // Create the new prefix
        res = asprintf(&final_prefix, "%s%s", config->global_prefix, type_prefix);
        assert(res != -1);
        config->prefixes_final[i] = final_prefix;
    }
    return 0;
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
    int ret = 0;
    // Initialize to the default values
    memcpy(config, &DEFAULT_CONFIG, sizeof(statsite_config));

    // If there is no filename, return now
    if (filename == NULL) {
        ret = 0;
        goto exit;
    }

    // Try to open the file
    int res = ini_parse(filename, config_callback, config);
    if (res == -1) {
        ret = -ENOENT;
    } else if (res) {
        syslog(LOG_ERR, "Failed to parse config on line: %d", res);
        ret = -res;
    }

exit:

    // Check for an unfinished histogram
    if (histogram_in_progress) {
        syslog(LOG_WARNING, "Unfinished configuration for section: %s", histogram_section);
        free(histogram_section);
        free(histogram_in_progress);
        histogram_in_progress = NULL;
        histogram_section = NULL;
    }

    if (sink_in_progress)
        sink_commit(config);

    /* Fill in a default sink if there is none specified */
    if (config->sink_configs == NULL) {
        config->sink_configs = (sink_config*)&DEFAULT_SINK;
    }

    return ret;
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

int sane_log_facility(char *log_facil, int *syslog_facility) {
    #define FACIL_MATCH(facil) (strcasecmp(facil, log_facil) == 0)

    if (FACIL_MATCH("local0")) {
        *syslog_facility = LOG_LOCAL0;
    } else if (FACIL_MATCH("local0")) {
        *syslog_facility = LOG_LOCAL0;
    } else if (FACIL_MATCH("local1")) {
        *syslog_facility = LOG_LOCAL1;
    } else if (FACIL_MATCH("local2")) {
        *syslog_facility = LOG_LOCAL2;
    } else if (FACIL_MATCH("local3")) {
        *syslog_facility = LOG_LOCAL3;
    } else if (FACIL_MATCH("local4")) {
        *syslog_facility = LOG_LOCAL4;
    } else if (FACIL_MATCH("local5")) {
        *syslog_facility = LOG_LOCAL5;
    } else if (FACIL_MATCH("local6")) {
        *syslog_facility = LOG_LOCAL6;
    } else if (FACIL_MATCH("local7")) {
        *syslog_facility = LOG_LOCAL7;
    } else if (FACIL_MATCH("user")) {
        *syslog_facility = LOG_USER;
    } else if (FACIL_MATCH("daemon")) {
        *syslog_facility = LOG_DAEMON;
    } else {
        syslog(LOG_ERR, "Invalid log facility!");
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

int sane_histograms(histogram_config *config) {
    while (config) {
        // Ensure sane upper / lower
        if (config->min_val >= config->max_val) {
            syslog(LOG_ERR, "Histogram min value must be less than max value! Prefix: %s", config->prefix);
            return 1;
        }

        // Check width
        if (config->bin_width <= 0) {
            syslog(LOG_ERR, "Histogram bin width must be greater than 0! Prefix: %s", config->prefix);
            return 1;
        }

        // Compute the number of bins
        // We divide the range by bin width, and add 2 for the less than min, and more than max bins
        config->num_bins = ((config->max_val - config->min_val) / config->bin_width) + 2;

        // Check that the count is sane
        if (config->num_bins > 1024) {
            syslog(LOG_ERR, "Histogram bin count cannot exceed 1024! Prefix: %s", config->prefix);
            return 1;
        } else if (config->num_bins > 128) {
            syslog(LOG_WARNING, "Histogram bin count very high! Bins: %d Prefix: %s",
                    config->num_bins, config->prefix);
        }

        // Inspect the next config
        config = config->next;
    }
    return 0;
}

int sane_set_precision(double eps, unsigned char *precision) {
    // Determine the minimum precision needed
    int minimum_prec = hll_precision_for_error(eps);
    if (minimum_prec < 0) {
        syslog(LOG_ERR, "Set epsilon must be between 0 and 1!");
        return 1;
    }

    // Check if the precision is within range
    if (minimum_prec < HLL_MIN_PRECISION) {
        syslog(LOG_ERR, "Set epsilon too high!");
        return 1;
    }
    if (minimum_prec > HLL_MAX_PRECISION) {
        syslog(LOG_ERR, "Set epsilon too low! Memory use would be prohibitive.");
        return 1;
    }

    // Warn if the precision is very high
    if (minimum_prec > 15) {
        syslog(LOG_WARNING, "Set epsilon low, high precision could \
cause increased memory use.");
    }

    *precision = minimum_prec;
    return 0;
}

int sane_quantiles(int num_quantiles, double quantiles[]) {
    for (int i=0; i < num_quantiles; i++) {
        if (quantiles[i] <= 0.0 || quantiles[i] >= 1.0) {
            syslog(LOG_ERR, "Quantiles must be between 0 and 1");
            return 1;
        }
    }
    return 0;
}

/**
 * Allocates memory for a new config structure
 * @return a pointer to a new config structure on success.
 */
statsite_config* alloc_config() {
    return calloc(1, sizeof(statsite_config));
}

/**
 * Frees memory associated with a previously allocated config structure
 * @arg config The config object to free.
 */
void free_config(statsite_config* config) {
    if (config->quantiles != default_quantiles) {
        free (config->quantiles);
    }
    free(config);
}


/**
 * Validates the configuration
 * @arg config The config object to validate.
 * @return 0 on success.
 */
int validate_config(statsite_config *config) {
    int res = 0;

    res |= sane_log_level(config->log_level, &config->syslog_log_level);
    res |= sane_log_facility(config->log_facility, &config->syslog_log_facility);
    res |= sane_timer_eps(config->timer_eps);
    res |= sane_flush_interval(config->flush_interval);
    res |= sane_histograms(config->hist_configs);
    res |= sane_set_precision(config->set_eps, &config->set_precision);
    res |= sane_quantiles(config->num_quantiles, config->quantiles);

    return res;
}

/**
 * Builds the radix tree for prefix matching
 * @return 0 on success
 */
int build_prefix_tree(statsite_config *config) {
    // Do nothing if there is no config
    if (!config->hist_configs)
        return 0;

    // Initialize the radix tree
    radix_tree *t = malloc(sizeof(radix_tree));
    config->histograms = t;
    int res = radix_init(t);
    if (res) goto ERR;

    // Add all the prefixes
    histogram_config *current = config->hist_configs;
    void **val;
    while (!res && current) {
        val = (void**)&current;
        res = radix_insert(t, current->prefix, val);
        current = current->next;
    }

    if (!res)
        return res;
ERR:
    free(t);
    return 1;
}

