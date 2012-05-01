import os
import os.path
import shutil
import socket
import subprocess
import sys
import tempfile
import time

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
    cmd = "cat > %s" % output

    # Write the configuration
    config_path = os.path.join(tmpdir, "config.cfg")
    conf = """[statsite]
flush_interval = 1
port = 8126
stream_cmd = %s
""" % cmd
    open(config_path, "w").write(conf)

    # Start the process
    proc = subprocess.Popen("./statsite -f %s" % config_path, shell=True)
    proc.poll()
    assert proc.returncode is None

    # Define a cleanup handler
    def cleanup():
        try:
            proc.terminate()
            proc.wait()
            shutil.rmtree(tmpdir)
        except:
            pass
    request.addfinalizer(cleanup)

    # Make a connection to the server
    connected = False
    for x in xrange(3):
        try:
            conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            conn.settimeout(1)
            conn.connect(("localhost", 8126))
            connected = True
            break
        except Exception, e:
            print e
            time.sleep(0.3)

    # Die now
    if not connected:
        raise EnvironmentError("Failed to connect!")

    # Make a second connection
    conn2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn2.settimeout(1)
    conn2.connect(("localhost", 8126))

    # Return the connection
    return conn, conn2, output


class TestInteg(object):
    def test_kv(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("tubez:100|kv\n")
        time.sleep(1)
        now = time.time()
        out = open(output).read()
        assert out in ("kv.tubez|100.000000|%d\n" % now, "kv.tubez|100.000000|%d\n" % (now - 1))

    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("foobar:100|c\n")
        server.sendall("foobar:200|c\n")
        server.sendall("foobar:300|c\n")
        time.sleep(1)
        now = time.time()
        out = open(output).read()
        assert out in ("counts.foobar|600.000000|%d\n" % now, "counts.foobar|600.000000|%d\n" % (now - 1))

    def test_meters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        msg = ""
        for x in xrange(100):
            msg += "noobs:%d|ms\n" % x
        server.sendall(msg)
        time.sleep(1)
        out = open(output).read()
        print out
        assert "timers.noobs.sum|4950" in out
        assert "timers.noobs.mean|49.500000" in out
        assert "timers.noobs.lower|0.000000" in out
        assert "timers.noobs.upper|99.000000" in out
        assert "timers.noobs.count|100" in out
        assert "timers.noobs.stdev|29.011492" in out
        assert "timers.noobs.median|49.000000" in out
        assert "timers.noobs.p95|95.000000" in out
        assert "timers.noobs.p99|99.000000" in out

if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))

