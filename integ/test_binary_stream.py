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
BIN_TYPES = {"kv": 1, "c": 2, "ms": 3}

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


def format_output(time, key, type, val_type, val):
    "Formats an response line. This is to check that we meet spec"
    prefix = BINARY_OUT_HEADER.pack(int(time), type, val_type, len(key) + 1, val)
    return prefix + key + "\0"


def wait_file(path, timeout=5):
    "Waits on a file to be make"
    start = time.time()
    while not os.path.isfile(path) and time.time() - start < timeout:
        time.sleep(0.1)
    if not os.path.isfile(path):
        raise Exception("Timed out waiting for file %s" % path)


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

        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum"], 4950)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["sum sq"], 328350)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["min"], 0)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["max"], 99)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["count"], 100)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["stddev"], 29.011492)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["mean"], 49.5)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P50"], 49)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P95"], 95)
        assert format_output(now, "noobs", BIN_TYPES["ms"], VAL_TYPE_MAP["P99"], 99)


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))

