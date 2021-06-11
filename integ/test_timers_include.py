import os
import os.path
import shutil
import socket
import subprocess
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
timers_include = MEAN,STDEV,SUM,SUM_SQ,LOWER,UPPER,SAMPLE_RATE

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
        except Exception as e:
            print(e)
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
    def test_counters(self, servers):
        "Tests adding kv pairs"
        server, _, output = servers
        server.sendall("foobar:100|ms\n")
        server.sendall("foobar:200|ms\n")
        server.sendall("foobar:300|ms\n")

        wait_file(output)
        out = open(output).read()
        assert "timers.foobar.count" not in out
        assert "timers.foobar.rate" not in out
        assert "timers.foobar.mean|200" in out
        assert "timers.foobar.stdev|100" in out
        assert "timers.foobar.sum|600" in out
        assert "timers.foobar.sum_sq|140000" in out
        assert "timers.foobar.lower|100" in out
        assert "timers.foobar.upper|300" in out
        assert "timers.foobar.median" not in out
        assert "timers.foobar.p50|200" in out
        assert "timers.foobar.sample_rate|3" in out


if __name__ == "__main__":
    sys.exit(pytest.main(args="-k TestInteg."))
