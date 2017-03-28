"""
Supports flushing metrics to graphite
"""
import sys
import socket
import logging
import pickle
import struct


class GraphiteStore(object):
    def __init__(self, host="localhost", port=2003, prefix="statsite.", attempts=3, protocol='lines'):
        """
        Implements an interface that allows metrics to be persisted to Graphite.
        Raises a :class:`ValueError` on bad arguments.

        :Parameters:
            - `host` : The hostname of the graphite server.
            - `port` : The port of the graphite server
            - `prefix` (optional) : A prefix to add to the keys. Defaults to 'statsite.'
            - `attempts` (optional) : The number of re-connect retries before failing.
        """
        # Convert the port to an int since its coming from a configuration file
        port = int(port)
        attempts = int(attempts)

        if port <= 0:
            raise ValueError("Port must be positive!")
        if attempts < 1:
            raise ValueError("Must have at least 1 attempt!")
        if protocol not in ["pickle", "lines"]:
            raise ValueError("Supported protocols are pickle, lines")

        self.logger = logging.getLogger("statsite.graphitestore")
        self.host = host
        self.port = port
        self.prefix = prefix
        self.attempts = attempts
        self.sock = self._create_socket()
        self.flush = self.flush_pickle if protocol == "pickle" else self.flush_lines

    def flush_lines(self, metrics):
        """
        Flushes the metrics provided to Graphite.

       :Parameters:
        - `metrics` : A list of "key|value|timestamp" strings.
        """
        if not metrics:
            return

        # Construct the output
        metrics = [m.split("|") for m in metrics if m and m.count("|") == 2]

        self.logger.info("Outputting %d metrics" % len(metrics))
        if self.prefix:
            lines = ["%s%s %s %s" % (self.prefix, k, v, ts) for k, v, ts in metrics]
        else:
            lines = ["%s %s %s" % (k, v, ts) for k, v, ts in metrics]
        data = "\n".join(lines) + "\n"

        # Serialize writes to the socket
        try:
            self._write_metric(data)
        except Exception:
            self.logger.exception("Failed to write out the metrics!")


    def flush_pickle(self, metrics):
        """
        Flushes the metrics provided to Graphite.

       :Parameters:
        - `metrics` : A list of "key|value|timestamp" strings.
        """
        if not metrics:
            return

        # transform a list of strings into the list of tuples that
        # pickle graphite interface supports, in the form of
        # (key, (timestamp, value))
        # http://graphite.readthedocs.io/en/latest/feeding-carbon.html#the-pickle-protocol
        metrics_fmt = []
        for m in metrics:
            if m.count("|") == 2:
                met = m.split("|")
                metrics_fmt.append((self.prefix + met[0], (met[2], met[1])))

        # do pickle the list of tuples
        # add the header the pickle protocol wants
        payload = pickle.dumps(metrics_fmt, protocol=2)
        header = struct.pack("!L", len(payload))
        message = header + payload

        self.logger.info("Outputting %d metrics" % len(metrics))

        try:
            self._write_metric(message)
        except Exception:
            self.logger.exception("Failed to write out the metrics!")

    def close(self):
        """
        Closes the connection. The socket will be recreated on the next
        flush.
        """
        try:
            if self.sock:
                self.sock.close()
        except Exception:
            self.logger.warning("Failed to close connection!")

    def _create_socket(self):
        """Creates a socket and connects to the graphite server"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect((self.host, self.port))
        except Exception:
            self.logger.error("Failed to connect!")
            sock = None
        return sock

    def _write_metric(self, metric):
        """Tries to write a string to the socket, reconnecting on any errors"""
        for attempt in xrange(self.attempts):
            if self.sock:
                try:
                    self.sock.sendall(metric)
                    return
                except socket.error:
                    self.logger.exception("Error while flushing to graphite. Reattempting...")

            self.sock = self._create_socket()

        self.logger.critical("Failed to flush to Graphite! Gave up after %d attempts." % self.attempts)


if __name__ == "__main__":
    # Initialize the logger
    logging.basicConfig()

    # Intialize from our arguments
    graphite = GraphiteStore(*sys.argv[1:])

    # Get all the inputs
    metrics = sys.stdin.read()

    # Flush
    graphite.flush(metrics.splitlines())
    graphite.close()
