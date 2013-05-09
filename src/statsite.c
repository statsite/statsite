/**
 * This is the main entry point into statsite.
 * We are responsible for parsing any commmand line
 * flags, reading the configuration, starting
 * the filter manager, and finally starting the
 * front ends.
 */
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "conn_handler.h"
#include "networking.h"

/**
 * By default we should run. Our signal
 * handler updates this variable to allow the
 * program to gracefully terminate.
 */
static int SHOULD_RUN = 1;

/**
 * Prints our usage to stderr
 */
void show_usage() {
    fprintf(stderr, "usage: statsite [-h] [-f filename]\n\
\n\
    -h : Displays this help info\n\
    -f : Reads the configuration from this file\n\
\n");
}

/**
 * Invoked to parse the command line options
 */
int parse_cmd_line_args(int argc, char **argv, char **config_file) {
    int enable_help = 0;

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "hf:w:")) != -1) {
        switch (c) {
            case 'h':
                enable_help = 1;
                break;
            case 'f':
                *config_file = optarg;
                break;
            case '?':
                if (optopt == 'f')
                    fprintf(stderr, "Option -%c requires a filename.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
        }
    }

    // Check if we need to show usage
    if (enable_help) {
        show_usage();
        return 1;
    }

    return 0;
}


/**
 * Initializes the syslog configuration
 */
void setup_syslog() {
    // If we are on a tty, log the errors out
    int flags = LOG_CONS|LOG_NDELAY|LOG_PID;
    if (isatty(1)) {
        flags |= LOG_PERROR;
    }
    openlog("statsite", flags, LOG_LOCAL0);
}


/**
 * Our registered signal handler, invoked
 * when we get signals such as SIGINT, SIGTERM.
 */
void signal_handler(int signum) {
    if (!SHOULD_RUN) {
        syslog(LOG_WARNING, "Received signal [%s] while exiting! Terminating", strsignal(signum));
        exit(1);
    }
    SHOULD_RUN = 0;  // Stop running now
    syslog(LOG_WARNING, "Received signal [%s]! Exiting...", strsignal(signum));
}


/**
 * Writes the pid to the configured pidfile
 */
int write_pidfile(char *pid_file, pid_t pid) {
    struct stat buf;
    int stat_res = stat(pid_file, &buf);
    if (stat_res == 0) {
        syslog(LOG_ERR, "pid file already exists! (%s)", pid_file);
        return 1;
    }

    FILE *file = fopen(pid_file, "w");
    if (!file) {
        syslog(LOG_ERR, "Failed to open pid file! (%s)", pid_file);
        return 1;
    }

    fprintf(file, "%d", pid);
    fclose(file);

    return 0;
}


int main(int argc, char **argv) {
    // Initialize syslog
    setup_syslog();

    // Parse the command line
    char *config_file = NULL;
    int parse_res = parse_cmd_line_args(argc, argv, &config_file);
    if (parse_res) return 1;

    // Parse the config file
    statsite_config *config = calloc(1, sizeof(statsite_config));
    int config_res = config_from_filename(config_file, config);
    if (config_res != 0) {
        syslog(LOG_ERR, "Failed to read the configuration file!");
        return 1;
    }

    // Validate the config file
    if (validate_config(config)) {
        syslog(LOG_ERR, "Invalid configuration!");
        return 1;
    }

    // Build the prefix tree
    if (build_prefix_tree(config)) {
        syslog(LOG_ERR, "Failed to build prefix tree!");
        return 1;
    }

    // Set the syslog mask
    setlogmask(config->syslog_log_level);

    // Daemonize
    if (config->daemonize) {
        pid_t pid, sid;
        int fd;
        syslog(LOG_INFO, "Daemonizing.");
        pid = fork();

        // Exit if we failed to fork
        if (pid < 0) {
            syslog(LOG_ERR, "Failed to fork() daemon!");
            return 1;
        }

        // Parent process returns
        if (pid) return 0;

        // Create a new session
        sid = setsid();
        if (sid < 0) {
            syslog(LOG_ERR, "Failed to set daemon SID!");
            return 1;
        }

        int write_pidfile_res = write_pidfile(config->pid_file, sid);
        if (write_pidfile_res) {
            syslog(LOG_ERR, "Failed to write pidfile. Terminating.");
            return 1;
        }

        if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
          dup2(fd, STDIN_FILENO);
          dup2(fd, STDOUT_FILENO);
          dup2(fd, STDERR_FILENO);
          if (fd > STDERR_FILENO) close(fd);
        }
    }

    // Log that we are starting up
    syslog(LOG_INFO, "Starting statsite.");

    // Initialize the networking
    statsite_networking *netconf = NULL;
    int net_res = init_networking(config, &netconf);
    if (net_res != 0) {
        syslog(LOG_ERR, "Failed to initialize networking!");
        return 1;
    }

    // Setup signal handlers
    signal(SIGPIPE, SIG_IGN);       // Ignore SIG_IGN
    signal(SIGHUP, SIG_IGN);        // Ignore SIG_IGN
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Join the networking loop, blocks until exit
    enter_networking_loop(netconf, &SHOULD_RUN);

    // Begin the shutdown/cleanup
    shutdown_networking(netconf);

    // Do the final flush
    final_flush();

    // If daemonized, remove the pid file
    if (config->daemonize && unlink(config->pid_file)) {
        syslog(LOG_ERR, "Failed to delete pid file!");
    }

    // Free our memory
    free(config);

    // Done
    return 0;
}
