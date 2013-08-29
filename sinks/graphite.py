"""
Supports flushing metrics to graphite
"""
import sys
import socket
import logging


class GraphiteStore(object):
    def __init__(self, host="localhost", port=2003, prefix="statsite", attempts=3):
        """
        Implements an interface that allows metrics to be persisted to Graphite.
        Raises a :class:`ValueError` on bad arguments.

        :Parameters:
            - `host` : The hostname of the graphite server.
            - `port` : The port of the graphite server
            - `prefix` (optional) : A prefix to add to the keys. Defaults to 'statsite'
            - `attempts` (optional) : The number of re-connect retries before failing.
        """
        # Convert the port to an int since its coming from a configuration file
        port = int(port)
        attempts = int(attempts)

        if port <= 0:
            raise ValueError("Port must be positive!")
        if attempts <= 1:
            raise ValueError("Must have at least 1 attempt!")

        self.host = host
        self.port = port
        self.prefix = prefix
        self.attempts = attempts
        self.sock = self._create_socket()
        self.logger = logging.getLogger("statsite.graphitestore")

    def flush(self, metrics):
        """
        Flushes the metrics provided to Graphite.

       :Parameters:
        - `metrics` : A list of (key,value,timestamp) tuples.
        """
        if not metrics:
            return

        # Construct the output
        metrics = [m.split("|") for m in metrics if m]
        self.logger.info("Outputting %d metrics" % len(metrics))
        if self.prefix:
            lines = ["%s.%s %s %s" % (self.prefix, k, v, ts) for k, v, ts in metrics]
        else:
            lines = ["%s %s %s" % (k, v, ts) for k, v, ts in metrics]
        data = "\n".join(lines) + "\n"

        # Serialize writes to the socket
        try:
            self._write_metric(data)
        except:
            self.logger.exception("Failed to write out the metrics!")

    def close(self):
        """
        Closes the connection. The socket will be recreated on the next
        flush.
        """
        self.sock.close()

    def _create_socket(self):
        """Creates a socket and connects to the graphite server"""
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((self.host, self.port))
        return sock

    def _write_metric(self, metric):
        """Tries to write a string to the socket, reconnecting on any errors"""
        for attempt in xrange(self.attempts):
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
