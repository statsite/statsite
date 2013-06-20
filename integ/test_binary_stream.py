"""
Integration testing for the binary streaming protocol.
This is for the backend, as opposed to the frontend binary
protocol.
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
COUNT_VAL = struct.Struct("<I")
BIN_TYPES = {"kv": 1, "c": 2, "ms": 3, "set": 4, "g": 5}

BINARY_OUT_HEADER = struct.Struct("<QBBHd")
BINARY_OUT_LEN = 20

VAL_TYPE_MAP = {
    "kv": 0,
    "sum": 1,
    "sum sq": 2,
    "mean": 3,
    "count": 4,
    "stddev": 5,
    "min": 6,
    "max": 7,
    "hist_min": 8,
    "hist_bin": 9,
    "hist_max": 10,
    "percentile": 128,
}

# Pre-compute all the possible percentiles
for x in xrange(1, 100):
    VAL_TYPE_MAP["P%02d" % x] = 128 | x


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
binary_stream = yes

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


def format_output(time, key, type, val_type, val):
    "Formats an response line. This is to check that we meet spec"
    prefix = BINARY_OUT_HEADER.pack(int(time), type, val_type, len(key) + 1, val)
    return prefix + key + "\0"

def format_output_count(time, key, type, val_type, val, count):
    "Formats a response line that includes a count, for histograms"
    prefix = format_output(time, key, type, val_type, val)
    return prefix + COUNT_VAL.pack(count)

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
        assert out in (format_output(now, "tubez", BIN_TYPES["kv"], VAL_TYPE_MAP["kv"], 100),
                       format_output(now - 1, "tubez", BIN_TYPES["kv"], VAL_TYPE_MAP["kv"], 100))

    def test_gauges(self, servers):
        "Tests streaming gauges"
        server, _, output = servers
        server.sendall(format("g1", "g", 500))
        wait_file(output)
        now = time.time()
        out = open(output).read()
        assert out in (format_output(now, "g1", BIN_TYPES["g"], VAL_TYPE_MAP["kv"], 500),
                       format_output(now - 1, "g1", BIN_TYPES["g"], VAL_TYPE_MAP["kv"], 500))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall(format("foobar", "c", 100))
        server.sendall(format("foobar", "c", 200))
        server.sendall(format("foobar", "c", 300))
        wait_file(output)
        now = time.time()
        out = open(output).read()

        # Adjust for time drift
        if format_output(now - 1, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["sum"], 600) in out:
            now = now - 1

        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["sum"], 600) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["sum sq"], 140000) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["mean"], 200) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["count"], 3) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["stddev"], 100) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["min"], 100) in out
        assert format_output(now, "foobar", BIN_TYPES["c"], VAL_TYPE_MAP["max"], 300) in out

    def test_meters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
            msg += format("noobs", "ms", x)
        server.sendall(msg)
        wait_file(output)
        now = time.time()
        out = open(output).read()

        # Adjust for time drift
        if format_output(now - 1, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum"], 4950) in out:
            now = now - 1

        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum"], 4950) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum sq"], 328350) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["min"], 0) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["max"], 99) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["count"], 100) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["stddev"], 29.011491975882016) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["mean"], 49.5) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P50"], 49) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P95"], 95) in out
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P99"], 99) in out

    def test_histogram(self, servers):
        "Tests streaming of histogram values"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
            msg += format("has_hist.test", "ms", x)
        server.sendall(msg)
        wait_file(output)
        now = time.time()
        out = open(output).read()

        # Adjust for time drift
        if format_output(now - 1, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum"], 4950) in out:
            now = now - 1

        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_min"], 10, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 10, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 20, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 30, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 40, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 50, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 60, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 70, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_bin"], 80, 10) in out
        assert format_output_count(now, "has_hist.test", BIN_TYPES["ms"], VAL_TYPE_MAP["hist_max"], 90, 10) in out

    def test_sets(self, servers):
        "Tests adding sets"
        server, _, output = servers
        server.sendall(format_set("zip", "foo"))
        server.sendall(format_set("zip", "bar"))
        server.sendall(format_set("zip", "baz"))
        wait_file(output)
        now = time.time()
        out = open(output).read()

        # Adjust for time drift
        if format_output(now - 1, "zip", BIN_TYPES["set"], VAL_TYPE_MAP["sum"], 3) in out:
            now = now - 1
        assert format_output(now, "zip", BIN_TYPES["set"], VAL_TYPE_MAP["sum"], 3) in out


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))

