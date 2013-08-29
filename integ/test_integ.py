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
    port = random.randrange(10000, 65000)
    config_path = os.path.join(tmpdir, "config.cfg")
    conf = """[statsite]
flush_interval = 1
port = %d
udp_port = %d
stream_cmd = %s

[histogram1]
prefix=has_hist
min=10
max=90
width=10

""" % (port, port, cmd)
    open(config_path, "w").write(conf)

    # Start the process
    proc = subprocess.Popen("./statsite -f %s" % config_path, shell=True)
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

    # Make a connection to the server
    connected = False
    for x in xrange(3):
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


class TestInteg(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("tubez:100|kv\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_gauges(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall("g1:1|g\n")
        server.sendall("g1:50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.g1|50.000000|%d\n" % now, "gauges.g1|50.000000|%d\n" % (now - 1))

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall("gd:+50|g\n")
        server.sendall("gd:+50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|100.000000|%d\n" % now, "gauges.gd|100.000000|%d\n" % (now - 1))

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall("gd:-50|g\n")
        server.sendall("gd:-50|g\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|-100.000000|%d\n" % now, "gauges.gd|-100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("foobar:100|c\n")
        server.sendall("foobar:200|c\n")
        server.sendall("foobar:300|c\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_counters_sample(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("foobar:100|c|@0.1\n")
        server.sendall("foobar:200|c|@0.1\n")
        server.sendall("foobar:300|c|@0.1\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|6000.000000|%d\n" % now, "counts.foobar|6000.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
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
        assert "timers.noobs.p95|95.000000" in out
        assert "timers.noobs.p99|99.000000" in out

    def test_histogram(self, servers):
        "Tests adding keys with histograms"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
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
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("zip:foo|s\n")
        server.sendall("zip:bar|s\n")
        server.sendall("zip:baz|s\n")

        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("sets.zip|3|%d\n" % now, "sets.zip|3|%d\n" % (now - 1))


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
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_counters_sample(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall("foobar:100|c|@0.1\n")
        server.sendall("foobar:200|c|@0.1\n")
        server.sendall("foobar:300|c|@0.1\n")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|6000.000000|%d\n" % now, "counts.foobar|6000.000000|%d\n" % (now - 1))

    def test_counters_no_newlines(self, servers):
        "Tests adding counters without a trailing new line"
        _, server, output = servers
        server.sendall("zip:100|c")
        server.sendall("zip:200|c")
        server.sendall("zip:300|c")
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.zip|600.000000|%d\n" % now, "counts.zip|600.000000|%d\n" % (now - 1))

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
        assert "timers.noobs.p95|95.000000" in out
        assert "timers.noobs.p99|99.000000" in out

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
            p = subprocess.Popen('./statsite -f %s' % fh.name, shell=True)
            time.sleep(0.3)
            yield port
        finally:
            p.kill()
            fh.close()

    def islistening(self, addr, port, command='statsite'):
        try:
            cmd = 'lsof -FnPc -nP -i @%s:%s' % (addr, port)
            out = subprocess.check_output(cmd, shell=True)
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
        try:
            port = random.randrange(10000, 65000)
            p = subprocess.Popen(['nc', '-l', '127.0.0.1', str(port)])
            with self.run('127.0.0.1', port):
                assert not self.islistening('127.0.0.0', port), 'should not be listening'
        finally:
            p.kill()


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
