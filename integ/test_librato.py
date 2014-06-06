"""
Testing the librato sink special features
"""
import sys
import os
import sinks.librato
import json
import pprint
import tempfile
import time

try:
    import pytest
except ImportError:
    print >> sys.stderr, "Integ tests require pytests!"
    sys.exit(1)


class TestLibrato(object):
    def setup_method(self, method):
        self.sample_output = """\
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
        """

        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("""\
[librato]
email = john@example.com
token = 02ac4003c4fcd11bf9cee34e34263155dc7ba1906c322d167db6ab4b2cd2082b
source = localhost
        """)
        f.close()
        self.librato = sinks.librato.LibratoStore(f.name)
        os.unlink(f.name)

        self.librato.build(self.sample_output.splitlines())

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
