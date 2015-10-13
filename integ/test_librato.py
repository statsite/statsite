"""
Testing the librato sink special features
"""
import sys
import os
import sinks.librato
import json
import tempfile
import time

try:
    import pytest
except ImportError:
    print >> sys.stderr, "Integ tests require pytests!"
    sys.exit(1)


def build_librato(options={}):
    options.setdefault("statsite_output", """\
counts.active_sessions|1.000000|1401577507
timers.query.sum|1017.000000|1401577507
timers.query.sum_sq|1034289.000000|1401577507
timers.query.mean|1017.000000|1401577507
timers.query.lower|1017.000000|1401577507
timers.query.upper|1017.000000|1401577507
timers.query.count|1|1401577507
timers.query.stdev|0.000000|1401577507
timers.query.median|1017.000000|1401577507
timers.query.p95|1017.000000|1401577507
timers.query.p99|1017.000000|1401577507
timers.query.rate|16.950000|1401577507\
    """)

    f = tempfile.NamedTemporaryFile(delete=False)
    f.write(build_librato_config(options))
    f.close()
    librato = sinks.librato.LibratoStore(f.name)
    os.unlink(f.name)

    librato.build(options["statsite_output"].splitlines())
    return librato


def build_librato_config(options):
    config = """\
[librato]
email = john@example.com
token = 02ac4003c4fcd11bf9cee34e34263155dc7ba1906c322d167db6ab4b2cd2082b
source = localhost\
    """

    if "source_regex" in options:
        config += "\nsource_regex = %s" % (options["source_regex"])
    if "source_prefix" in options:
        config += "\nsource_prefix = %s" % (options["source_prefix"])

    return config


class TestLibrato(object):
    def setup_method(self, method):
        self.librato = build_librato()

    def test_gauge_specific_params_on_timers(self):
        expected_output = {
            "name":         "query",
            "source":       "localhost",
            "measure_time": 1401577507,
            "count":        1.0,
            "sum":          1017.0,
            "max":          1017.0,
            "min":          1017.0,
            "sum_squares":  1034289.0,
        }
        assert expected_output == self.librato.gauges["query\tlocalhost"]

    def test_counts_send_as_gauges(self):
        expected_output = {
            "name":         "active_sessions",
            "source":       "localhost",
            "measure_time": 1401577507,
            "value":        1.0,
        }
        assert expected_output == self.librato.gauges["active_sessions\tlocalhost"]

    def test_source_regex_can_match_more_than_beginning_of_metric(self):
        self.librato = build_librato({
            "statsite_output": "counts.baby-animals.source__puppy-cam-1__.active_sessions|1.000000|1401577507",
            "source_regex": "\.source__(.*?)__",
        })

        expected_output = {
            "name":         "baby-animals.active_sessions",
            "source":       "puppy-cam-1",
            "measure_time": 1401577507,
            "value":        1.0,
        }

        assert expected_output == self.librato.gauges["baby-animals.active_sessions\tpuppy-cam-1"]

    def test_source_prefix(self):
        self.librato = build_librato({
            "statsite_output": "counts.baby-animals.source__puppy-cam-1__.active_sessions|1.000000|1401577507",
            "source_regex": "\.source__(.*?)__",
            "source_prefix": "production",
        })

        expected_output = {
            "name":         "baby-animals.active_sessions",
            "source":       "production.puppy-cam-1",
            "measure_time": 1401577507,
            "value":        1.0,
        }

        assert expected_output == self.librato.gauges["baby-animals.active_sessions\tproduction.puppy-cam-1"]
