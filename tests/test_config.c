#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "config.h"

START_TEST(test_config_get_default)
{
    statsite_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);
    fail_unless(config.tcp_port == 8125);
    fail_unless(config.udp_port == 8125);
    fail_unless(config.parse_stdin == false);
    fail_unless(strcmp(config.log_level, "DEBUG") == 0);
    fail_unless(config.syslog_log_level == LOG_DEBUG);
    fail_unless(config.timer_eps == 1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);

}
END_TEST

START_TEST(test_config_bad_file)
{
    statsite_config config;
    int res = config_from_filename("/tmp/does_not_exist", &config);
    fail_unless(res == -ENOENT);

    // Should get the defaults...
    fail_unless(config.tcp_port == 8125);
    fail_unless(config.udp_port == 8125);
    fail_unless(config.parse_stdin == false);
    fail_unless(strcmp(config.log_level, "DEBUG") == 0);
    fail_unless(config.syslog_log_level == LOG_DEBUG);
    fail_unless(config.timer_eps == 1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);
}
END_TEST

START_TEST(test_config_empty_file)
{
    int fh = open("/tmp/zero_file", O_CREAT|O_RDWR, 0777);
    fchmod(fh, 777);
    close(fh);

    statsite_config config;
    int res = config_from_filename("/tmp/zero_file", &config);
    fail_unless(res == 0);

    // Should get the defaults...
    fail_unless(config.tcp_port == 8125);
    fail_unless(config.udp_port == 8125);
    fail_unless(config.parse_stdin == false);
    fail_unless(strcmp(config.log_level, "DEBUG") == 0);
    fail_unless(config.syslog_log_level == LOG_DEBUG);
    fail_unless(config.timer_eps == 1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);

    unlink("/tmp/zero_file");
}
END_TEST

START_TEST(test_config_basic_config)
{
    int fh = open("/tmp/basic_config", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
port = 10000\n\
udp_port = 10001\n\
parse_stdin = true\n\
flush_interval = 120\n\
timer_eps = 0.005\n\
set_eps = 0.03\n\
stream_cmd = foo\n\
log_level = INFO\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    statsite_config config;
    int res = config_from_filename("/tmp/basic_config", &config);
    fail_unless(res == 0);

    // Should get the config
    fail_unless(config.tcp_port == 10000);
    fail_unless(config.udp_port == 10001);
    fail_unless(config.parse_stdin == true);
    fail_unless(strcmp(config.log_level, "INFO") == 0);
    fail_unless(config.timer_eps == 0.005);
    fail_unless(config.set_eps == 0.03);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);

    unlink("/tmp/basic_config");
}
END_TEST

START_TEST(test_validate_default_config)
{
    statsite_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    res = validate_config(&config);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_validate_bad_config)
{
    statsite_config config;
    int res = config_from_filename(NULL, &config);
    fail_unless(res == 0);

    // Set an absurd eps, should fail
    config.timer_eps = 1.0;

    res = validate_config(&config);
    fail_unless(res == 1);
}
END_TEST

START_TEST(test_join_path_no_slash)
{
    char *s1 = "/tmp/path";
    char *s2 = "file";
    char *s3 = join_path(s1, s2);
    fail_unless(strcmp(s3, "/tmp/path/file") == 0);
}
END_TEST

START_TEST(test_join_path_with_slash)
{
    char *s1 = "/tmp/path/";
    char *s2 = "file";
    char *s3 = join_path(s1, s2);
    fail_unless(strcmp(s3, "/tmp/path/file") == 0);
}
END_TEST

START_TEST(test_sane_log_level)
{
    int log_lvl;
    fail_unless(sane_log_level("DEBUG", &log_lvl) == 0);
    fail_unless(sane_log_level("debug", &log_lvl) == 0);
    fail_unless(sane_log_level("INFO", &log_lvl) == 0);
    fail_unless(sane_log_level("info", &log_lvl) == 0);
    fail_unless(sane_log_level("WARN", &log_lvl) == 0);
    fail_unless(sane_log_level("warn", &log_lvl) == 0);
    fail_unless(sane_log_level("ERROR", &log_lvl) == 0);
    fail_unless(sane_log_level("error", &log_lvl) == 0);
    fail_unless(sane_log_level("CRITICAL", &log_lvl) == 0);
    fail_unless(sane_log_level("critical", &log_lvl) == 0);
    fail_unless(sane_log_level("foo", &log_lvl) == 1);
    fail_unless(sane_log_level("BAR", &log_lvl) == 1);
}
END_TEST

START_TEST(test_sane_timer_eps)
{
    fail_unless(sane_timer_eps(-0.01) == 1);
    fail_unless(sane_timer_eps(1.0) == 1);
    fail_unless(sane_timer_eps(0.5) == 1);
    fail_unless(sane_timer_eps(0.1) == 0);
    fail_unless(sane_timer_eps(0.05) == 0);
    fail_unless(sane_timer_eps(0.01) == 0);
    fail_unless(sane_timer_eps(0.001) == 0);
    fail_unless(sane_timer_eps(0.0001) == 0);
    fail_unless(sane_timer_eps(0.00001) == 0);
}
END_TEST

START_TEST(test_sane_flush_interval)
{
    fail_unless(sane_flush_interval(-1) == 1);
    fail_unless(sane_flush_interval(0) == 1);
    fail_unless(sane_flush_interval(60) == 0);
    fail_unless(sane_flush_interval(120) == 0);
    fail_unless(sane_flush_interval(86400) == 0);
}
END_TEST

START_TEST(test_sane_histograms)
{
    histogram_config c = {"foo", 100, 200, 10, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 0);
    fail_unless(c.num_bins == 12);

    c = (histogram_config){"foo", 200, 200, 10, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 1);

    c = (histogram_config){"foo", 100, 50, 10, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 1);

    c = (histogram_config){"foo", 0, 1000, 0.5, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 1);

    c = (histogram_config){"foo", 0, 1000, 2, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 0);
    fail_unless(c.num_bins == 502);

    c = (histogram_config){"foo", 0, 100, 5, 0, NULL, 0};
    fail_unless(sane_histograms(&c) == 0);
    fail_unless(c.num_bins == 22);
}
END_TEST

START_TEST(test_sane_set_eps)
{
    unsigned char prec;
    fail_unless(sane_set_precision(-0.01, &prec) == 1);
    fail_unless(sane_set_precision(1.0, &prec) == 1);
    fail_unless(sane_set_precision(0.5, &prec) == 1);

    fail_unless(sane_set_precision(0.2, &prec) == 0);
    fail_unless(prec == 5);
    fail_unless(sane_set_precision(0.1, &prec) == 0);
    fail_unless(prec == 7);
    fail_unless(sane_set_precision(0.03, &prec) == 0);
    fail_unless(prec == 11);
    fail_unless(sane_set_precision(0.01, &prec) == 0);
    fail_unless(prec == 14);

    fail_unless(sane_set_precision(0.001, &prec) == 1);
}
END_TEST



START_TEST(test_config_histograms)
{
    int fh = open("/tmp/histogram_basic", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
port = 10000\n\
udp_port = 10001\n\
flush_interval = 120\n\
timer_eps = 0.005\n\
stream_cmd = foo\n\
log_level = INFO\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n\
\n\
[histogram_n1]\n\
prefix=api.\n\
min=0\n\
max=100\n\
width=10\n\
\n\
[histogram_n2]\n\
prefix=site.\n\
min=0\n\
max=200\n\
width=10\n\
\n\
[histogram_n3]\n\
prefix=\n\
min=-500\n\
max=500\n\
width=25\n\
\n\
";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    statsite_config config;
    int res = config_from_filename("/tmp/histogram_basic", &config);
    fail_unless(res == 0);

    // Should get the config
    fail_unless(config.tcp_port == 10000);
    fail_unless(config.udp_port == 10001);
    fail_unless(strcmp(config.log_level, "INFO") == 0);
    fail_unless(config.timer_eps == 0.005);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);

    histogram_config *c = config.hist_configs;
    fail_unless(c != NULL);
    fail_unless(strcmp(c->prefix, "") == 0);
    fail_unless(c->min_val == -500);
    fail_unless(c->max_val == 500);
    fail_unless(c->bin_width == 25);

    c = c->next;
    fail_unless(strcmp(c->prefix, "site.") == 0);
    fail_unless(c->min_val == 0);
    fail_unless(c->max_val == 200);
    fail_unless(c->bin_width == 10);

    c = c->next;
    fail_unless(strcmp(c->prefix, "api.") == 0);
    fail_unless(c->min_val == 0);
    fail_unless(c->max_val == 100);
    fail_unless(c->bin_width == 10);

    unlink("/tmp/histogram_basic");
}
END_TEST

START_TEST(test_build_radix)
{
    statsite_config config;
    int res = config_from_filename(NULL, &config);

    histogram_config c1 = {"foo", 100, 200, 10, 0, NULL, 0};
    histogram_config c2 = {"bar", 100, 200, 10, 0, NULL, 0};
    histogram_config c3 = {"baz", 100, 200, 10, 0, NULL, 0};
    config.hist_configs = &c1;
    c1.next = &c2;
    c2.next = &c3;

    fail_unless(build_prefix_tree(&config) == 0);
    fail_unless(config.histograms != NULL);

    histogram_config *conf = NULL;
    fail_unless(radix_search(config.histograms, "baz", (void**)&conf) == 0);
    fail_unless(conf == &c3);
}
END_TEST

