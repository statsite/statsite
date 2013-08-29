"""
Integration testing for the binary protocol instead
of the ASCII protocol.
"""
import os
import os.path
import socket
import subprocess
import sys
import tempfile
import time
import random
import struct

try:
    import pytest
except ImportError:
    print >> sys.stderr, "Integ tests require pytests!"
    sys.exit(1)


BINARY_HEADER = struct.Struct("<BBHd")
BINARY_SET_HEADER = struct.Struct("<BBHH")
BIN_TYPES = {"kv": 1, "c": 2, "ms": 3, "set": 4, "g": 5, "delta": 6}


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


def format(key, type, val):
    "Formats a binary message for statsite"
    key = str(key)
    key_len = len(key) + 1
    type_num = BIN_TYPES[type]
    header = BINARY_HEADER.pack(170, type_num, key_len, float(val))
    mesg = header + key + "\0"
    return mesg

def format_set(key, val):
    "Formats a binary set message for statsite"
    key = str(key)
    key_len = len(key) + 1
    val = str(val)
    val_len = len(val) + 1
    type_num = BIN_TYPES["set"]
    header = BINARY_SET_HEADER.pack(170, type_num, key_len, val_len)
    mesg = "".join([header, key, "\0", val, "\0"])
    return mesg


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
        server.sendall(format("tubez", "kv", 100))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_gauges(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall(format("g1", "g", 1))
        server.sendall(format("g1", "g", 120))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.g1|120.000000|%d\n" % now, "gauges.g1|120.000000|%d\n" % (now - 1))

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall(format("gd", "delta", 50))
        server.sendall(format("gd", "delta", 50))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|100.000000|%d\n" % now, "gauges.gd|100.000000|%d\n" % (now - 1))

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        server, _, output = servers
        server.sendall(format("gd", "delta", -50))
        server.sendall(format("gd", "delta", -50))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|-100.000000|%d\n" % now, "gauges.gd|-100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall(format("foobar", "c", 100))
        server.sendall(format("foobar", "c", 200))
        server.sendall(format("foobar", "c", 300))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
            msg += format("noobs", "ms", x)
        server.sendall(msg)
        wait_file(output)
        out = open(output).read()
        print out
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
        server, _, output = servers
        server.sendall(format_set("zip", "foo"))
        server.sendall(format_set("zip", "bar"))
        server.sendall(format_set("zip", "baz"))
        wait_file(output)
        out = open(output).read()
        assert "sets.zip|3|" in out


class TestIntegUDP(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall(format("tubez", "kv", 100))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_gauges(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall(format("g1", "g", 1))
        server.sendall(format("g1", "g", 120))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.g1|120.000000|%d\n" % now, "gauges.g1|120.000000|%d\n" % (now - 1))

    def test_gauges_delta(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall(format("gd", "delta", 50))
        server.sendall(format("gd", "delta", 50))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|100.000000|%d\n" % now, "gauges.gd|100.000000|%d\n" % (now - 1))

    def test_gauges_delta_neg(self, servers):
        "Tests adding gauges"
        _, server, output = servers
        server.sendall(format("gd", "delta", -50))
        server.sendall(format("gd", "delta", -50))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("gauges.gd|-100.000000|%d\n" % now, "gauges.gd|-100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        server.sendall(format("foobar", "c", 100))
        server.sendall(format("foobar", "c", 200))
        server.sendall(format("foobar", "c", 300))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        _, server, output = servers
        msg = ""
        for x in xrange(100):
            msg += format("noobs", "ms", x)
        server.sendall(msg)
        wait_file(output)
        out = open(output).read()
        print out
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
        server.sendall(format_set("zip", "foo"))
        server.sendall(format_set("zip", "bar"))
        server.sendall(format_set("zip", "baz"))
        wait_file(output)
        out = open(output).read()
        assert "sets.zip|3|" in out


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
