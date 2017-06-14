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
timers.query.mean|1017.000000|1401577507
timers.query.lower|1017.000000|1401577507
timers.query.upper|1017.000000|1401577507
timers.query.count|1|1401577507
timers.query.stdev|0.000000|1401577507
timers.query.median|1017.000000|1401577507
timers.query.p95|1017.000000|1401577507
timers.query.p99|1017.000000|1401577507
timers.query.rate|16.950000|1401577507
counts.query.tags#tag1=value1|16.000000|1401577507
timers.tags_many#tag1=value1,tag2=value2.p90|16.000000|1401577507
timers.tags_many_dots#tag1=value1,tag2=value.now.p90|16.000000|1401577507\
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
token = 02ac4003c4fcd11bf9cee34e34263155dc7ba1906c322d167db6ab4b2cd2082b\
    """
    if "source" in options:
        config += "\nsource = %s" % (options["source"])
    if "source_regex" in options:
        config += "\nsource_regex = %s" % (options["source_regex"])
    if "source_prefix" in options:
        config += "\nsource_prefix = %s" % (options["source_prefix"])

    if "host" in options:
        config += "\nhost = %s" % (options["host"])
    
    if "write_to_legacy" in options:
        config += "\nwrite_to_legacy = %s" % (options["write_to_legacy"])
    else:
        config += "\nwrite_to_legacy = True" 

    return config


class TestLibrato(object):
    def setup_method(self, method):
        self.librato = build_librato({"source": "localhost"})

    def test_gauge_specific_params_on_timers(self):
        expected_output = {
            "name":         "query",
            "source":       "localhost",
            "measure_time": 1401577507,
            "count":        1.0,
            "sum":          1017.0,
            "max":          1017.0,
            "min":          1017.0
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
            "source": "localhost"
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
            "source": "localhost"
        })

        expected_output = {
            "name":         "baby-animals.active_sessions",
            "source":       "production.puppy-cam-1",
            "measure_time": 1401577507,
            "value":        1.0,
        }

        assert expected_output == self.librato.gauges["baby-animals.active_sessions\tproduction.puppy-cam-1"]
    
    def test_measurements_with_tags(self):
        expected_output = {
            "name":         "query.tags",
            "time":         1401577507,
            "value":        16.0,
            "tags":         { "source": "localhost", "tag1": "value1" }
        }
        
        # No top level sources allowed here...
        assert False == self.librato.measurements.has_key("source")
        assert expected_output == self.librato.measurements["query.tags\tlocalhost"]

    def test_measurements_with_tags_no_source(self):
        self.librato = build_librato()
        expected_output = {
            "name":         "query.tags",
            "time":         1401577507,
            "value":        16.0,
            "tags":         { "tag1": "value1" }
        }

        # No top level sources allowed here...
        assert False == self.librato.measurements.has_key("source")
        assert expected_output == self.librato.measurements["query.tags"]

    def test_measurements_with_suffix(self):
        expected_output = {
            "name":         "tags_many.p90",
            "time":         1401577507,
            "value" :         16.0,
            "tags":         { "source": "localhost", "tag1": "value1", "tag2": "value2" }
        }

        assert expected_output == self.librato.measurements["tags_many.p90\tlocalhost"]
        
    def test_measurements_with_period_in_tag_value_and_suffix(self):
        expected_output = {
            "name":         "tags_many_dots.p90",
            "time":         1401577507,
            "value" :         16.0,
            "tags":         { "source": "localhost", "tag1": "value1", "tag2": "value.now" }
        }
        assert expected_output == self.librato.measurements["tags_many_dots.p90\tlocalhost"]
    
    def test_write_to_legacy(self):
        expected_tags_output = {
            "name":         "active_sessions",
            "time":         1401577507,
            "value":        1.0,
            "tags":         { "source": "localhost" }
        }

        expected_legacy_output = {
            "name":         "active_sessions",
            "source":       "localhost",
            "measure_time": 1401577507,
            "value":        1.0,
        }
        assert expected_tags_output == self.librato.measurements["active_sessions\tlocalhost"]
        assert expected_legacy_output == self.librato.gauges["active_sessions\tlocalhost"]

    def test_measurements_with_host(self):
        self.librato = build_librato({"host": "localhost"})
        expected_output = {
            "name":         "query.tags",
            "time":         1401577507,
            "value":        16.0,
            "tags":         { "tag1": "value1" }
        }
        
        assert {"host": "localhost" } == self.librato.tags
        assert expected_output == self.librato.measurements["query.tags"]
