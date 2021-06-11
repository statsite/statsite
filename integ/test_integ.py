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
    port = random.randrange(10000, 65000)
    config_path = os.path.join(tmpdir, "config.cfg")
    conf = """[statsite]
flush_interval = 1
port = %d
udp_port = %d
stream_cmd = %s
quantiles = 0.5, 0.9, 0.95, 0.99

[histogram1]
prefix=has_hist
min=10
max=90
width=10

""" % (port, port, cmd)
    open(config_path, "w").write(conf)

    # Start the process
    proc = subprocess.Popen(['./statsite', '-f', config_path])
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

    # Make a connection to the server
    connected = False
    for x in range(3):
        try:
            conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            conn.settimeout(1)
            conn.connect(("localhost", port))
            connected = True
            break
        except Exception, e:
            print e
            time.sleep(0.5)

    # Die now
    if not connected:
        raise EnvironmentError("Failed to connect!")

    # Make a second connection
    conn2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    conn2.connect(("localhost", port))

    # Return the connection
    return conn, conn2, output


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
        server, _, output = servers

        server.sendall("tubez1:100|kv\n")

        server.sendall("tubez2;env=prod:50|kv\n") # Tagged metric

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
        server, _, output = servers
        
        server.sendall("g1:1|g\n")
        server.sendall("g1:50|g\n")

        server.sendall("g2;env=prod:4|g\n") # Tagged metric
        server.sendall("g2;env=prod:5|g\n")

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
        server, _, output = servers
        
        server.sendall("gd1:+50|g\n")
        server.sendall("gd1:+50|g\n")

        server.sendall("gd2;env=prod:+4|g\n") # Tagged metric
        server.sendall("gd2;env=prod:+5|g\n")

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
        server, _, output = servers
       
        server.sendall("gd1:-50|g\n")
        server.sendall("gd1:-50|g\n")

        server.sendall("gd2;env=prod:+4|g\n") # Tagged metric
        server.sendall("gd2;env=prod:-5|g\n")

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
        server, _, output = servers

        server.sendall("foobar1:100|c\n")
        server.sendall("foobar1:200|c\n")
        server.sendall("foobar1:300|c\n")

        server.sendall("foobar2;env=prod:2|c\n") # Tagged metric
        server.sendall("foobar2;env=prod:3|c\n")

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
        server, _, output = servers
        
        server.sendall("foobar1:100|c|@0.1\n")
        server.sendall("foobar1:200|c|@0.1\n")
        server.sendall("foobar1:300|c|@0.1\n")

        server.sendall("foobar2;env=prod:2|c|@0.1\n") # Tagged metric
        server.sendall("foobar2;env=prod:3|c|@0.1\n")

        wait_file(output)
        now = time.time()

        test_set = {
            "counts.foobar1|6000.000000|",
            "counts.foobar2;env=prod|50.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_meters_alias(self, servers):
        "Tests adding timing data with the 'h' alias"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
            msg += "val:%d|h\n" % x
        server.sendall(msg)

        wait_file(output)
        out = open(output).read()
        assert "timers.val.sum|4950" in out
        assert "timers.val.sum_sq|328350" in out
        assert "timers.val.mean|49.500000" in out
        assert "timers.val.lower|0.000000" in out
        assert "timers.val.upper|99.000000" in out
        assert "timers.val.count|100" in out
        assert "timers.val.stdev|29.011492" in out
        assert "timers.val.median|49.000000" in out
        assert "timers.val.p90|90.000000" in out
        assert "timers.val.p95|95.000000" in out
        assert "timers.val.p99|99.000000" in out
        assert "timers.val.rate|4950" in out
        assert "timers.val.sample_rate|100" in out

    def test_meters(self, servers):
        "Tests adding meters"
        server, _, output = servers

        msg = ""
        for x in range(100):
            msg += "noobs1:%d|ms\n" % x
            msg += "noobs2;env=prod:%d|ms\n" % x # Tagged metric

        server.sendall(msg)

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
            "timers.noobs1.p90|90.000000|",
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
            "timers.noobs2.p90;env=prod|90.000000|",
            "timers.noobs2.p95;env=prod|95.000000|",
            "timers.noobs2.p99;env=prod|99.000000|"
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_timers_with_rate(self, servers):
        "Tests timers with sampling rate"
        server, _, output = servers
        msg = ""
        for x in range(100):
            msg += "withrate:%d|ms|@0.5\n" % x
        server.sendall(msg)

        wait_file(output)
        out = open(output).read()
        assert "timers.withrate.sum|4950" in out
        assert "timers.withrate.sum_sq|328350" in out
        assert "timers.withrate.mean|49.500000" in out
        assert "timers.withrate.lower|0.000000" in out
        assert "timers.withrate.upper|99.000000" in out
        assert "timers.withrate.count|200" in out
        assert "timers.withrate.stdev|29.011492" in out
        assert "timers.withrate.median|49.000000" in out
        assert "timers.withrate.p90|90.000000" in out
        assert "timers.withrate.p95|95.000000" in out
        assert "timers.withrate.p99|99.000000" in out
        assert "timers.withrate.rate|4950" in out
        assert "timers.withrate.sample_rate|200.0" in out

    def test_histogram(self, servers):
        "Tests adding keys with histograms"
        server, _, output = servers
        msg = ""
        for x in range(100):
            msg += "has_hist.test:%d|ms\n" % x
        server.sendall(msg)

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
        "Tests adding sets"
        server, _, output = servers
        
        server.sendall("zip1:foo|s\n")
        server.sendall("zip1:bar|s\n")
        server.sendall("zip1:baz|s\n")

        server.sendall("zip2;env=prod:foo|s\n") # Tagged metric
        server.sendall("zip2;env=prod:bar|s\n")

        wait_file(output)
        now = time.time()

        now = time.time()

        test_set = {
            "sets.zip1|3|",
            "sets.zip2;env=prod|2|",
        }
        times_list = [ now, now - 1 ]

        verify(output, test_set, times_list)

    def test_double_parsing(self, servers):
        "Tests string to double parsing"
        server, _, output = servers
        server.sendall("int1:1|c\n")
        server.sendall("decimal1:1.0|c\n")
        server.sendall("decimal2:2.3456789|c\n")
        server.sendall("scientific1:1.0e5|c\n")
        server.sendall("scientific2:2.0e05|c\n")
        server.sendall("scientific3:3.0E05|c\n")
        server.sendall("scientific4:4.0e-5|c\n")
        server.sendall("underflow1:1.964393875E-314|c\n")

        wait_file(output)
        out = open(output).read()
        assert "counts.int1|1.000000|" in out
        assert "counts.decimal1|1.000000|" in out
        assert "counts.decimal2|2.345679|" in out
        assert "counts.scientific1|100000.000000|" in out
        assert "counts.scientific2|200000.000000|" in out
        assert "counts.scientific3|300000.000000|" in out
        assert "counts.scientific4|0.000040|" in out
        assert "counts.underflow1|" not in out

#    def test_condensed(self, servers):
#        "Tests adding condensed stats"
#        _, server, output = servers
#        server.sendall("foo:4|c:5|ms:3|ms:7|g\n")
#
#        wait_file(output)
#        now = time.time()
#        out = open(output).read()
#        assert ("counts.foo|4.000") in out
#        assert ("timers.foo.sum|8.000") in out
#        assert ("gauges.foo|7.000") in out


class TestIntegUDP(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("tubez:100|kv\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_gauges(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall("g1:1|g\n")
        server.sendall("g1:50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.g1|50.000000|%d\n" % now, "gauges.g1|50.000000|%d\n" % (now - 1))

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall("gd:+50|g\n")
        server.sendall("gd:+50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|100.000000|%d\n" % now, "gauges.gd|100.000000|%d\n" % (now - 1))

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall("gd:-50|g\n")
        server.sendall("gd:-50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|-100.000000|%d\n" % now, "gauges.gd|-100.000000|%d\n" % (now - 1))

    def test_bad_kv(self, servers):
        "Tests adding a bad value, followed by a valid kv pair"
        _, server, output = servers
        server.sendall("this is junk data\n")
        server.sendall("tubez:100|kv\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("foobar:100|c\n")
        server.sendall("foobar:200|c\n")
        server.sendall("foobar:300|c\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % (now),
                       "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_counters_signed(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("foobar:+100|c\n")
        server.sendall("foobar:+200|c\n")
        server.sendall("foobar:-50|c\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|250.000000|%d\n" % (now),
                       "counts.foobar|250.000000|%d\n" % (now - 1))

    def test_counters_sample(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("foobar:100|c|@0.1\n")
        server.sendall("foobar:200|c|@0.1\n")
        server.sendall("foobar:300|c|@0.1\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|6000.000000|%d\n" % (now),
                       "counts.foobar|6000.000000|%d\n" % (now - 1))

    def test_wrong_protocol_in_a_batch(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("foobar10c\nfoobar:10|c\nfoobar:10|c\n")
        server.sendall("foobar:c\nfoobar:10|c\nfoobar:10|c\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|40.000000|%d\n" % (now),
                       "counts.foobar|40.000000|%d\n" % (now - 1))

    def test_counters_no_newlines(self, servers):
        "Tests adding counters without a trailing new line"
        _, server, output = servers
        server.sendall("zip:100|c")
        server.sendall("zip:200|c")
        server.sendall("zip:300|c")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.zip|600.000000|%d\n" % (now),
                       "counts.zip|600.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        msg = ""
        for x in xrange(100):
            msg += "noobs:%d|ms\n" % x
        server.sendall(msg)
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
        assert "timers.noobs.p90|90.000000" in out
        assert "timers.noobs.p95|95.000000" in out
        assert "timers.noobs.p99|99.000000" in out
        assert "timers.noobs.rate|4950" in out
        assert "timers.noobs.sample_rate|100" in out

    def test_sets(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("zip:foo|s\n")
        server.sendall("zip:bar|s\n")
        server.sendall("zip:baz|s\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("sets.zip|3|%d\n" % now, "sets.zip|3|%d\n" % (now - 1))


class TestIntegBindAddress(object):
    @contextlib.contextmanager
    def run(self, addr, port=None):
        port = port if port else random.randrange(10000, 65000)
        fh = tempfile.NamedTemporaryFile()

        conf = '''\
        [statsite]
        port = %d
        udp_port = %s
        bind_address = %s\n'''
        fh.write(textwrap.dedent(conf % (port, port, addr)))
        fh.flush()

        try:
            p = subprocess.Popen(['./statsite', '-f', fh.name])
            time.sleep(0.3)
            yield port
        finally:
            p.kill()
            fh.close()

    def islistening(self, addr, port, command='statsite'):
        try:
            cmd = ['lsof', '-FnPc', '-nP', '-i', '@%s:%s' % (addr, port)]
            out = subprocess.check_output(cmd)
        except subprocess.CalledProcessError:
            return False

        return (command in out) and ('PTCP' in out) and ('PUDP' in out)

    def test_ipv4_localhost(self):
        with self.run('127.0.0.1') as port:
            assert self.islistening('127.0.0.1', port), 'not listening'

    def test_ipv4_any(self):
        with self.run('0.0.0.0') as port:
            assert self.islistening('0.0.0.0', port), 'not listening'

    def test_ipv4_bogus(self):
        with self.run('a.b.c.d') as port:
            assert not self.islistening('0.0.0.0', port), 'should not be listening'
        with self.run('1.0.1.0') as port:
            assert not self.islistening('1.0.1.0', port), 'should not be listening'

    def test_ipv4_used(self):
        p = None
        try:
            port = random.randrange(10000, 65000)
            p = subprocess.Popen(['nc', '-l', '127.0.0.1', str(port)])
            with self.run('127.0.0.1', port):
                assert not self.islistening('127.0.0.0', port), 'should not be listening'
        finally:
            if not p is None:
                p.kill()


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
