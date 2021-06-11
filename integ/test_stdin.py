import os
import os.path
import socket
import textwrap
import shutil
import subprocess
import contextlib
import sys
import tempfile
import time
import random

try:
    import pytest
except ImportError:
    print >> sys.stderr, "Integ tests require pytests!"
    sys.exit(1)


@pytest.fixture
def servers(request):
    "Returns a new APIHandler with a filter manager"
    # Create tmpdir and delete after
    tmpdir = tempfile.mkdtemp()

    # Make the command
    output = "%s/output" % tmpdir
    cmd = "cat >> %s" % output

    # Write the configuration
    config_path = os.path.join(tmpdir, "config.cfg")
    conf = """[statsite]
flush_interval = 1
port = 0
udp_port = 0
parse_stdin = yes
stream_cmd = %s

[histogram1]
prefix=has_hist
min=10
max=90
width=10

""" % (cmd)
    open(config_path, "w").write(conf)

    # Start the process
    proc = subprocess.Popen(['./statsite', '-f', config_path], stdin=subprocess.PIPE)
    proc.poll()
    assert proc.returncode is None

    # Define a cleanup handler
    def cleanup():
        try:
            proc.kill()
            proc.wait()
            shutil.rmtree(tmpdir)
        except:
            print(proc)
            pass
    request.addfinalizer(cleanup)

    # Return the connection
    return proc.stdin, output


def wait_file(path, timeout=5):
    "Waits on a file to be make"
    start = time.time()
    while not os.path.isfile(path) and time.time() - start < timeout:
        time.sleep(0.1)
    if not os.path.isfile(path):
        raise Exception("Timed out waiting for file %s" % path)
    while os.path.getsize(path) == 0 and time.time() - start < timeout:
        time.sleep(0.1)


def verify_line(line, test_set, times_list):
    for test in test_set:
        if line.startswith(test):
            for t in times_list:
                v = "%s%d\n" % (test, t)
                if line == v:
                    test_set.remove(test)
                    return
        else:
            l = line.split('|')
            t = test.split('|')
            if l[0] == t[0]:
                assert line == test, "value in line mismatch with test sets"

    # line not founded
    assert line in test_set, "line not found in test sets"

def verify_lines(lines, test_set, times_list):
    for line in lines:
        verify_line(line, test_set, times_list)

    # check for not founded test sets
    assert not test_set, "test sets not found in out"

def verify(output, test_set, times_list):
    lines = open(output).readlines()
    verify_lines(lines, test_set, times_list)


class TestInteg(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        server, output = servers

        server.write("tubez1:100|kv\n")

        server.write("tubez2;env=prod:50|kv\n") # Tagged metric

        wait_file(output)
        now = time.time()

        test_set = {
            "kv.tubez1|100.000000|",
            "kv.tubez2;env=prod|50.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_gauges(self, servers):
        "Tests adding gauges"
        server, output = servers

        server.write("g1:1|g\n")
        server.write("g1:50|g\n")

        server.write("g2;env=prod:4|g\n") # Tagged metric
        server.write("g2;env=prod:5|g\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "gauges.g1|50.000000|",
            "gauges.g2;env=prod|5.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        server, output = servers

        server.write("gd1:+50|g\n")
        server.write("gd1:+50|g\n")

        server.write("gd2;env=prod:+4|g\n") # Tagged metric
        server.write("gd2;env=prod:+5|g\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "gauges.gd1|100.000000|",
            "gauges.gd2;env=prod|9.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        server, output = servers

        server.write("gd1:-50|g\n")
        server.write("gd1:-50|g\n")

        server.write("gd2;env=prod:+4|g\n") # Tagged metric
        server.write("gd2;env=prod:-5|g\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "gauges.gd1|-100.000000|",
            "gauges.gd2;env=prod|-1.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_counters(self, servers):
        "Tests adding counters"
        server, output = servers

        server.write("foobar1:100|c\n")
        server.write("foobar1:200|c\n")
        server.write("foobar1:300|c\n")

        server.write("foobar2;env=prod:2|c\n") # Tagged metric
        server.write("foobar2;env=prod:3|c\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "counts.foobar1|600.000000|",
            "counts.foobar2;env=prod|5.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_counters_sample(self, servers):
        "Tests adding counters sample"
        server, output = servers

        server.write("foobar1:100|c|@0.1\n")
        server.write("foobar1:200|c|@0.1\n")
        server.write("foobar1:300|c|@0.1\n")

        server.write("foobar2;env=prod:2|c|@0.1\n") # Tagged metric
        server.write("foobar2;env=prod:3|c|@0.1\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "counts.foobar1|6000.000000|",
            "counts.foobar2;env=prod|50.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_meters(self, servers):
        "Tests adding meters"
        server, output = servers
        msg = ""
        for x in range(100):
            msg += "noobs1:%d|ms\n" % x
            msg += "noobs2;env=prod:%d|ms\n" % x # Tagged metric

        server.write(msg)

        wait_file(output)
        now = time.time()

        test_set = {
            "timers.noobs1.sum|4950.000000|",
            "timers.noobs1.sum_sq|328350.000000|",
            "timers.noobs1.mean|49.500000|",
            "timers.noobs1.lower|0.000000|",
            "timers.noobs1.upper|99.000000|",
            "timers.noobs1.count|100|",
            "timers.noobs1.rate|4950.000000|",
            "timers.noobs1.sample_rate|100.000000|",
            "timers.noobs1.stdev|29.011492|",
            "timers.noobs1.median|49.000000|",
            "timers.noobs1.p50|49.000000|",
            "timers.noobs1.p95|95.000000|",
            "timers.noobs1.p99|99.000000|",
            # Tagged metric
            "timers.noobs2.sum;env=prod|4950.000000|",
            "timers.noobs2.sum_sq;env=prod|328350.000000|",
            "timers.noobs2.mean;env=prod|49.500000|",
            "timers.noobs2.lower;env=prod|0.000000|",
            "timers.noobs2.upper;env=prod|99.000000|",
            "timers.noobs2.count;env=prod|100|",
            "timers.noobs2.rate;env=prod|4950.000000|",
            "timers.noobs2.sample_rate;env=prod|100.000000|",
            "timers.noobs2.stdev;env=prod|29.011492|",
            "timers.noobs2.median;env=prod|49.000000|",
            "timers.noobs2.p50;env=prod|49.000000|",
            "timers.noobs2.p95;env=prod|95.000000|",
            "timers.noobs2.p99;env=prod|99.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_histogram(self, servers):
        "Tests adding keys with histograms"
        server, output = servers
        msg = ""
        for x in range(100):
            msg += "has_hist.test1:%d|ms\n" % x
            msg += "has_hist.test2;env=prod:%d|ms\n" % x * 10 # Tagged metric
        server.write(msg)

        wait_file(output)

        out = open(output).read()

        assert "timers.has_hist.test1.histogram.bin_<10.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_10.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_20.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_30.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_40.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_50.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_60.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_70.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_80.00|10" in out
        assert "timers.has_hist.test1.histogram.bin_>90.00|10" in out

        # Tagged metric
        assert "timers.has_hist.test2.histogram.bin_<10.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_10.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_20.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_30.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_40.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_50.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_60.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_70.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_80.00;env=prod|10" in out
        assert "timers.has_hist.test2.histogram.bin_>90.00;env=prod|10" in out

    def test_sets(self, servers):
        "Tests adding sets"
        server, output = servers

        server.write("zip1:foo|s\n")
        server.write("zip1:bar|s\n")
        server.write("zip1:baz|s\n")

        server.write("zip2;env=prod:foo|s\n") # Tagged metric
        server.write("zip2;env=prod:bar|s\n")

        wait_file(output)
        now = time.time()

        now = time.time()

        test_set = {
            "sets.zip1|3|",
            "sets.zip2;env=prod|2|",
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
