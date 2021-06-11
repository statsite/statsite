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
flush_interval = 2
port = %d
udp_port = %d
stream_cmd = %s
extended_counters = true

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


@pytest.fixture
def servers_nonlegacy(request):
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
flush_interval = 2
port = %d
udp_port = %d
stream_cmd = %s
extended_counters = true
legacy_extended_counters = false

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
        "Tests adding counters with (legacy behaviour)"
        server, _, output = servers
        server.sendall("foobar:100|c\n")
        server.sendall("foobar:200|c\n")
        server.sendall("foobar:300|c\n")

        wait_file(output)
        out = open(output).read()
        assert "counts.foobar.count|3" in out
        assert "counts.foobar.rate|300" in out

    def test_counters_sample(self, servers):
        "Tests adding counters with sampling (legacy behaviour)"
        server, _, output = servers
        server.sendall("foobar:100|c|@0.1\n")
        server.sendall("foobar:200|c|@0.1\n")
        server.sendall("foobar:300|c|@0.1\n")

        wait_file(output)
        out = open(output).read()
        assert "counts.foobar.count|30" in out
        assert "counts.foobar.rate|3000" in out

    def test_counters_legacy(self, servers_nonlegacy):
        "Tests adding counters"
        server, _, output = servers_nonlegacy
        server.sendall("foobar:100|c\n")
        server.sendall("foobar:200|c\n")
        server.sendall("foobar:300|c\n")

        wait_file(output)
        out = open(output).read()
        assert "counts.foobar.count|600" in out
        assert "counts.foobar.rate|300" in out

    def test_counters_sample_legacy(self, servers_nonlegacy):
        "Tests adding counters with sampling"
        server, _, output = servers_nonlegacy
        server.sendall("foobar:100|c|@0.1\n")
        server.sendall("foobar:200|c|@0.1\n")
        server.sendall("foobar:300|c|@0.1\n")

        wait_file(output)
        out = open(output).read()
        assert "counts.foobar.count|6000" in out
        assert "counts.foobar.rate|3000" in out
