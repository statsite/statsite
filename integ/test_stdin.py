import os
import os.path
import socket
import textwrap
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


def pytest_funcarg__servers(request):
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
    proc = subprocess.Popen("./statsite -f %s" % config_path, shell=True, stdin=subprocess.PIPE)
    proc.poll()
    assert proc.returncode is None

    # Define a cleanup handler
    def cleanup():
        try:
            proc.kill()
            proc.wait()
            #shutil.rmtree(tmpdir)
        except:
            print proc
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


class TestInteg(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        server, output = servers
        server.write("tubez:100|kv\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_gauges(self, servers):
        "Tests adding gauges"
        server, output = servers
        server.write("g1:1|g\n")
        server.write("g1:50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.g1|50.000000|%d\n" % now, "gauges.g1|50.000000|%d\n" % (now - 1))

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        server, output = servers
        server.write("gd:+50|g\n")
        server.write("gd:+50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|100.000000|%d\n" % now, "gauges.gd|100.000000|%d\n" % (now - 1))

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        server, output = servers
        server.write("gd:-50|g\n")
        server.write("gd:-50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|-100.000000|%d\n" % now, "gauges.gd|-100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, output = servers
        server.write("foobar:100|c\n")
        server.write("foobar:200|c\n")
        server.write("foobar:300|c\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_counters_sample(self, servers):
        "Tests adding kv pairs"
        server, output = servers
        server.write("foobar:100|c|@0.1\n")
        server.write("foobar:200|c|@0.1\n")
        server.write("foobar:300|c|@0.1\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|6000.000000|%d\n" % now, "counts.foobar|6000.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        server, output = servers
        msg = ""
        for x in xrange(100):
            msg += "noobs:%d|ms\n" % x
        server.write(msg)

        wait_file(output)
        out = open(output).read()
        assert "timers.noobs.sum|4950" in out
        assert "timers.noobs.sum_sq|328350" in out
        assert "timers.noobs.mean|49.500000" in out
        assert "timers.noobs.lower|0.000000" in out
        assert "timers.noobs.upper|99.000000" in out
        assert "timers.noobs.count|100" in out
        assert "timers.noobs.stdev|29.011492" in out
        assert "timers.noobs.median|49.000000" in out
        assert "timers.noobs.p95|95.000000" in out
        assert "timers.noobs.p99|99.000000" in out

    def test_histogram(self, servers):
        "Tests adding keys with histograms"
        server, output = servers
        msg = ""
        for x in xrange(100):
            msg += "has_hist.test:%d|ms\n" % x
        server.write(msg)

        wait_file(output)
        out = open(output).read()
        assert "timers.has_hist.test.histogram.bin_<10.00|10" in out
        assert "timers.has_hist.test.histogram.bin_10.00|10" in out
        assert "timers.has_hist.test.histogram.bin_20.00|10" in out
        assert "timers.has_hist.test.histogram.bin_30.00|10" in out
        assert "timers.has_hist.test.histogram.bin_40.00|10" in out
        assert "timers.has_hist.test.histogram.bin_50.00|10" in out
        assert "timers.has_hist.test.histogram.bin_60.00|10" in out
        assert "timers.has_hist.test.histogram.bin_70.00|10" in out
        assert "timers.has_hist.test.histogram.bin_80.00|10" in out
        assert "timers.has_hist.test.histogram.bin_>90.00|10" in out

    def test_sets(self, servers):
        "Tests adding kv pairs"
        server, output = servers
        server.write("zip:foo|s\n")
        server.write("zip:bar|s\n")
        server.write("zip:baz|s\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("sets.zip|3|%d\n" % now, "sets.zip|3|%d\n" % (now - 1))

if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
