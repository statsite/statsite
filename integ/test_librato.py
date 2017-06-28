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
counts.requests.2xx#environment=stg|16.000000|1401577507
timers.request#tag1=value1,tag2=value2.p90|16.000000|1401577507\
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


    # TAG-BASED MEASUREMENTS

    def test_parse_tags(self):
        # multipart defaults to False
        metric, tags = self.librato.parse_tags("foo#role=db,env=prod,one=two")
        assert metric == 'foo'
        assert tags == {"role": "db", "env": "prod", "one": "two"}

    def test_parse_tags_multipart(self):
        metric, tags = self.librato.parse_tags("foo#role=db,env=prod,one=two", True)
        assert metric == 'foo'
        assert tags == {"role": "db", "env": "prod", "one": "two"}

    def test_parse_tags_no_tags(self):
        # multipart defaults to False
        metric, tags = self.librato.parse_tags("foo")
        assert metric == 'foo'
        assert tags == {}

    def test_measurements_with_tags(self):
        self.librato = build_librato({
            "statsite_output": "counts.requests.2xx#user_id=123,environment=stg|42|1401577507",
        })
        expected_output = {
            "name": "requests.2xx",
            "time": 1401577507,
            "value": 42,
            "tags": { "environment": "stg", "user_id": "123"}
        }

        # No top level sources allowed here...
        assert False == self.librato.measurements.has_key("source")
        assert False == self.librato.tags.has_key("source")
        assert expected_output == self.librato.measurements["requests.2xx"]

    def test_host_tag_autopopulated(self):
        # 'host' is automatically defaulted to `hostname` unless specified in the config
        assert self.librato.tags.has_key("host")
    
    def test_measurements_with_tags_and_source(self):
        self.librato = build_librato({
            "statsite_output": "counts.requests.2xx#environment=stg|42|1401577507",
            "source": "myhost"
        })
        expected_output = {
            "name": "requests.2xx",
            "time": 1401577507,
            "value": 42,
            "tags": { "source": "myhost", "environment": "stg" }
        }
        
        # This should default to `hostname`
        assert self.librato.tags.has_key("host")
        # No top level sources allowed here...
        assert False == self.librato.measurements.has_key("source")
        assert expected_output == self.librato.measurements["requests.2xx\tmyhost"]


    def test_metric_with_different_tagsets(self):
        data = """\
counts.requests.2xx#environment=stg,user_id=123|42|1401577507
counts.requests.2xx#environment=stg,user_id=456|43|1401577507
counts.requests.2xx#environment=stg|85|1401577507
"""
        self.librato = build_librato({"statsite_output": data})
        expected_output = [
            {
                "name": "requests.2xx",
                "time": 1401577507,
                "value": 42.0,
                "tags": { "environment": "stg", "user_id": "123"}
            },
            {
                "name": "requests.2xx",
                "time": 1401577507,
                "value": 43,
                "tags": { "environment": "stg", "user_id": "456"}
            },
            {
                "name": "requests.2xx",
                "time": 1401577507,
                "value": 85,
                "tags": { "environment": "stg"}
            }
        ]

        result = self.librato.measurements["requests.2xx"]
        print(expected_output)
        print(result)
        assert expected_output == result

    def test_timer_with_percentile_suffix(self):
        # Statsite will tack the p90 onto the actual metric name before we get it
        self.librato = build_librato({
            "statsite_output": "timers.request#env=prod,tag2=value2.p90|16.000000|1401577507"
        })
        expected_output = {
            "name": "request.p90",
            "time": 1401577507,
            "value": 16.0,
            "tags": { "env": "prod", "tag2": "value2" }
        }

        assert expected_output == self.librato.measurements["request.p90"]

    def test_timer_with_multiple_dotted_tag_values(self):
        metric = "timers.request#env=prod,tag1=value1.foo,tag2=value2.bar,tag3=value3.baz|16.123|1401577507"
        self.librato = build_librato({
            "statsite_output": metric
        })
        expected_output = {
            "name": "request.baz",
            "time": 1401577507,
            "value": 16.123,
            "tags": { "env": "prod", "tag1": "value1.foo", "tag2": "value2.bar", "tag3": "value3" }
        }

        print(self.librato.measurements)
        assert expected_output == self.librato.measurements["request.baz"]

    def test_timer_with_arbitrary_suffix(self):
        self.librato = build_librato({
            "statsite_output": "timers.request#env=prod,tag2=value2.foo|16.123|1401577507"
        })
        expected_output = {
            "name": "request.foo",
            "time": 1401577507,
            "value": 16.123,
            "tags": { "env": "prod", "tag2": "value2" }
        }

        assert expected_output == self.librato.measurements["request.foo"]

    def test_gauges_with_dotted_tags(self):
        self.librato = build_librato({
            "statsite_output": "gauges.request#env=prod,tag2=foo.bar|16.000000|1401577507"
        })
        expected_output = {
            "name": "request",
            "time": 1401577507,
            "value": 16.0,
            "tags": { "env": "prod", "tag2": "foo.bar" }
        }

        assert expected_output == self.librato.measurements["request"]
        
    def test_measurements_with_period_in_tag_value_and_suffix(self):
        self.librato = build_librato({
            "statsite_output": "timers.tags_many_dots#tag1=value1,tag2=value.dotted.p90|16.000000|1401577507"
        })
        expected_output = {
            "name": "tags_many_dots.p90",
            "time": 1401577507,
            "value": 16.0,
            "tags": { "tag1": "value1", "tag2": "value.dotted" }
        }
        assert expected_output == self.librato.measurements["tags_many_dots.p90"]
    
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
            "name":         "requests.2xx",
            "time":         1401577507,
            "value":        16.0,
            "tags":         { "environment": "stg" }
        }
        
        assert {"host": "localhost" } == self.librato.tags
        assert expected_output == self.librato.measurements["requests.2xx"]
