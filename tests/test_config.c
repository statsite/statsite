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
    fail_unless(config.syslog_log_facility == LOG_LOCAL0);
    fail_unless(config.timer_eps == (double)1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);
    fail_unless(config.extended_counters == false);
    fail_unless(config.prefix_binary_stream == false);
    fail_unless(config.num_quantiles == 3);
    fail_unless(config.quantiles[0] == 0.5);
    fail_unless(config.quantiles[1] == 0.95);
    fail_unless(config.quantiles[2] == 0.99);
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
    fail_unless(config.syslog_log_facility == LOG_LOCAL0);
    fail_unless(config.timer_eps == (double)1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);
    fail_unless(config.extended_counters == false);
    fail_unless(config.prefix_binary_stream == false);
    fail_unless(config.num_quantiles == 3);
    fail_unless(config.quantiles[0] == 0.5);
    fail_unless(config.quantiles[1] == 0.95);
    fail_unless(config.quantiles[2] == 0.99);

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
    fail_unless(config.syslog_log_facility == LOG_LOCAL0);
    fail_unless(config.timer_eps == (double)1e-2);
    fail_unless(strcmp(config.stream_cmd, "cat") == 0);
    fail_unless(config.flush_interval == 10);
    fail_unless(config.daemonize == false);
    fail_unless(config.binary_stream == false);
    fail_unless(strcmp(config.pid_file, "/var/run/statsite.pid") == 0);
    fail_unless(config.input_counter == NULL);
    fail_unless(config.extended_counters == false);
    fail_unless(config.prefix_binary_stream == false);
    fail_unless(config.num_quantiles == 3);
    fail_unless(config.quantiles[0] == 0.5);
    fail_unless(config.quantiles[1] == 0.95);
    fail_unless(config.quantiles[2] == 0.99);

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
log_facility = local3\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n\
extended_counters = true\n\
prefix_binary_stream = true\n\
quantiles = 0.5, 0.90, 0.95, 0.99\n";
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
    fail_unless(strcmp(config.log_facility, "local3") == 0);
    fail_unless(config.timer_eps == (double)0.005);
    fail_unless(config.set_eps == (double)0.03);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);
    fail_unless(config.extended_counters == true);
    fail_unless(config.prefix_binary_stream == true);
    fail_unless(config.num_quantiles == 4);
    fail_unless(config.quantiles[0] == 0.5);
    fail_unless(config.quantiles[1] == 0.90);
    fail_unless(config.quantiles[2] == 0.95);
    fail_unless(config.quantiles[3] == 0.99);

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

START_TEST(test_sane_log_facility)
{
    int log_facil;
    fail_unless(sane_log_facility("local0", &log_facil) == 0);
    fail_unless(sane_log_facility("local1", &log_facil) == 0);
    fail_unless(sane_log_facility("local2", &log_facil) == 0);
    fail_unless(sane_log_facility("local3", &log_facil) == 0);
    fail_unless(sane_log_facility("local4", &log_facil) == 0);
    fail_unless(sane_log_facility("local5", &log_facil) == 0);
    fail_unless(sane_log_facility("local6", &log_facil) == 0);
    fail_unless(sane_log_facility("local7", &log_facil) == 0);
    fail_unless(sane_log_facility("user", &log_facil) == 0);
    fail_unless(sane_log_facility("daemon", &log_facil) == 0);
    fail_unless(sane_log_facility("foo", &log_facil) == 1);
    fail_unless(sane_log_facility("BAR", &log_facil) == 1);
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

START_TEST(test_sane_quantiles)
{
    double good[] = { 0.01, 0.99 };
    double low[]  = { 0.00, 0.99 };
    double high[] = { 0.99, 1.00 };

    fail_unless(sane_quantiles(2, good) == 0);
    fail_unless(sane_quantiles(2, low)  == 1);
    fail_unless(sane_quantiles(2, high) == 1);
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
log_facility = local0\n\
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
    fail_unless(config.timer_eps == (double)0.005);
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

START_TEST(test_sane_prefixes)
{
    int fh = open("/tmp/sane_prefixes", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
use_type_prefix = true\n\
kv_prefix = keyVALUE.1-2_3\n\
gauges_prefix =\n\
counts_prefix=foo.counts.bar.\n\
";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    // Should get the config
    statsite_config config;
    int res = config_from_filename("/tmp/sane_prefixes", &config);
    fail_unless(res == 0);
    // And prepare prefixes
    res = prepare_prefixes(&config);
    fail_unless(res == 0);

    // Values from config
    fail_unless(strcmp(config.prefixes_final[KEY_VAL], "keyVALUE.1-2_3") == 0);
    fail_unless(strcmp(config.prefixes_final[GAUGE], "") == 0);
    fail_unless(strcmp(config.prefixes_final[COUNTER], "foo.counts.bar.") == 0);

    // Default values
    fail_unless(strcmp(config.prefixes_final[TIMER], "timers.") == 0);
    fail_unless(strcmp(config.prefixes_final[SET], "sets.") == 0);

    unlink("/tmp/sane_prefixes");

    fh = open("/tmp/sane_prefixes", O_CREAT|O_RDWR, 0777);

    buf = "[statsite]\n\
use_type_prefix = 0\n\
kv_prefix = keyVALUE.1-2_3\n\
gauges_prefix =\n\
counts_prefix=foo.sets.bar";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    // Should get the config
    res = config_from_filename("/tmp/sane_prefixes", &config);
    fail_unless(res == 0);

    // And prepare prefixes
    res = prepare_prefixes(&config);
    fail_unless(res == 0);

    // Values from config
    fail_unless(strcmp(config.prefixes_final[KEY_VAL], "") == 0);
    fail_unless(strcmp(config.prefixes_final[GAUGE], "") == 0);
    fail_unless(strcmp(config.prefixes_final[COUNTER], "") == 0);

    // Default values
    fail_unless(strcmp(config.prefixes_final[TIMER], "") == 0);
    fail_unless(strcmp(config.prefixes_final[SET], "") == 0);

    unlink("/tmp/sane_prefixes");

    fh = open("/tmp/sane_prefixes", O_CREAT|O_RDWR, 0777);

    buf = "[statsite]\n\
use_type_prefix = 0\n\
global_prefix = statsite.\n\
kv_prefix = keyVALUE.1-2_3\n\
gauges_prefix =\n\
counts_prefix=foo.sets.bar";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    // Should get the config
    res = config_from_filename("/tmp/sane_prefixes", &config);
    fail_unless(res == 0);

    // And prepare prefixes
    res = prepare_prefixes(&config);
    fail_unless(res == 0);

    // Values from config
    fail_unless(strcmp(config.prefixes_final[KEY_VAL], "statsite.") == 0);
    fail_unless(strcmp(config.prefixes_final[GAUGE], "statsite.") == 0);
    fail_unless(strcmp(config.prefixes_final[COUNTER], "statsite.") == 0);

    // Default values
    fail_unless(strcmp(config.prefixes_final[TIMER], "statsite.") == 0);
    fail_unless(strcmp(config.prefixes_final[SET], "statsite.") == 0);

    unlink("/tmp/sane_prefixes");
}
END_TEST

START_TEST(test_sane_global_prefix)
{
    int fh = open("/tmp/global_prefix", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
port = 10000\n\
udp_port = 10001\n\
parse_stdin = true\n\
flush_interval = 120\n\
timer_eps = 0.005\n\
set_eps = 0.03\n\
stream_cmd = foo\n\
log_level = INFO\n\
log_facility = level0\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n\
global_prefix = statsd.\n\
use_type_prefix = 1\n\
kv_prefix = keyvalue.\n\
gauges_prefix =\n\
sets_prefix=foo.sets.bar.";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    // Should get the config
    statsite_config config;
    int res = config_from_filename("/tmp/global_prefix", &config);
    fail_unless(res == 0);
    // And prepare prefixes
    res = prepare_prefixes(&config);
    fail_unless(res == 0);

    // Ensure that we didn't affect other values
    fail_unless(config.tcp_port == 10000);
    fail_unless(config.udp_port == 10001);
    fail_unless(config.parse_stdin == true);
    fail_unless(strcmp(config.log_level, "INFO") == 0);
    fail_unless(config.timer_eps == (double)0.005);
    fail_unless(config.set_eps == (double)0.03);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);

    // Values from config
    fail_unless(strcmp(config.prefixes_final[KEY_VAL], "statsd.keyvalue.") == 0);
    fail_unless(strcmp(config.prefixes_final[GAUGE], "statsd.") == 0);
    fail_unless(strcmp(config.prefixes_final[SET], "statsd.foo.sets.bar.") == 0);

    // Default values
    fail_unless(strcmp(config.prefixes_final[TIMER], "statsd.timers.") == 0);
    fail_unless(strcmp(config.prefixes_final[COUNTER], "statsd.counts.") == 0);
    unlink("/tmp/global_prefix");
}
END_TEST

START_TEST(test_basic_sink)
{
    int fh = open("/tmp/ss_sink_basic", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
port = 10000\n\
udp_port = 10001\n\
flush_interval = 120\n\
timer_eps = 0.005\n\
stream_cmd = foo\n\
log_level = INFO\n\
log_facility = local0\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n\
\n\
[sink_stream_main]\n\
binary=true\n\
command=cat\n\
";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    statsite_config config;
    int res = config_from_filename("/tmp/ss_sink_basic", &config);
    fail_unless(res == 0);

    // Should get the config
    fail_unless(config.tcp_port == 10000);
    fail_unless(config.udp_port == 10001);
    fail_unless(strcmp(config.log_level, "INFO") == 0);
    fail_unless(config.timer_eps == (double)0.005);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);

    sink_config *c = config.sink_configs;
    fail_unless(c != NULL);
    fail_unless(c->type == SINK_TYPE_STREAM);

    sink_config_stream *cs = (sink_config_stream*)c;
    fail_unless(cs->binary_stream == true);
    fail_unless(strcmp(cs->stream_cmd, "cat") == 0);

    unlink("/tmp/ss_sink_basic");
}
END_TEST

START_TEST(test_multi_sink)
{
    int fh = open("/tmp/ss_sink_multi", O_CREAT|O_RDWR, 0777);
    char *buf = "[statsite]\n\
port = 10000\n\
udp_port = 10001\n\
flush_interval = 120\n\
timer_eps = 0.005\n\
stream_cmd = foo\n\
log_level = INFO\n\
log_facility = local0\n\
daemonize = true\n\
binary_stream = true\n\
input_counter = foobar\n\
pid_file = /tmp/statsite.pid\n\
\n\
[sink_stream_main]\n\
binary=true\n\
command=cat\n\
\n\
[sink_stream_other]\n\
binary=false\n\
command=not cat\n\
";
    write(fh, buf, strlen(buf));
    fchmod(fh, 777);
    close(fh);

    statsite_config config;
    int res = config_from_filename("/tmp/ss_sink_multi", &config);
    fail_unless(res == 0);

    // Should get the config
    fail_unless(config.tcp_port == 10000);
    fail_unless(config.udp_port == 10001);
    fail_unless(strcmp(config.log_level, "INFO") == 0);
    fail_unless(config.timer_eps == (double)0.005);
    fail_unless(strcmp(config.stream_cmd, "foo") == 0);
    fail_unless(config.flush_interval == 120);
    fail_unless(config.daemonize == true);
    fail_unless(config.binary_stream == true);
    fail_unless(strcmp(config.pid_file, "/tmp/statsite.pid") == 0);
    fail_unless(strcmp(config.input_counter, "foobar") == 0);

    sink_config *c = config.sink_configs;
    fail_unless(c != NULL);
    fail_unless(c->type == SINK_TYPE_STREAM);
    fail_unless(c->next != NULL);
    sink_config *c2 = c->next;
    
    sink_config_stream *cs = (sink_config_stream*)c;
    sink_config_stream *cs2 = (sink_config_stream*)c2;
    fail_unless(cs->binary_stream == false);
    fail_unless(strcmp(cs->stream_cmd, "not cat") == 0);
    fail_unless(cs2->binary_stream == true);
    fail_unless(strcmp(cs2->stream_cmd, "cat") == 0);

    unlink("/tmp/ss_sink_multi");
}
END_TEST
