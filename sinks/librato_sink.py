"""
Supports flushing metrics to librato


Installation
In your shell:
$ easy_install librato-metrics
or
$ pip install librato-metrics
"""
import sys
import logging
import librato

class LibratoSink(object):
    def __init__(self, user, token, prefix="statsite", attempts=3):
        """
        Implements an interface that allows metrics to be persisted to Librato.

        The API requires HTTP basic authentication for every request. All requests must be sent over HTTPS.
        Authentication is accomplished with a user and token pair. 

        :Parameters:

            - `user` : User is the email address that you used to create your Librato Metrics account
            - `token` : The API token
            - `prefix` (optional) : A prefix to add to the keys. Defaults to 'statsite'
            - `attempts` (optional) : The number of re-connect retries before failing.
        """ 
        attempts = int(attempts)
        if attempts < 1:
            raise ValueError("Must have at least 1 attempt!")

        self.user = user
        self.token = token
        self.api = librato.connect(self.user, self.token)        

        self.prefix = prefix
        self.attempts = attempts
        self.logger = logging.getLogger("statsite.libratostore")
        self.logger.setLevel(logging.INFO)

    def close(self):
        """
        Closes the connection. The socket will be recreated on the next
        flush.
        """
        self.logger.debug("Close")

    def flush(self, metrics):
        """
        Flushes the metrics provided to Librato.

       :Parameters:
        - `metrics` : A list of (key,value,timestamp) tuples.
        """

        q_len = 0
        q = self.api.new_queue()
        for m in metrics:
            m = m.strip()
            if not m: continue

            (k, v, ts) = m.split('|')
            key = k.replace('>', 'gt').replace('<', 'le')
            value = float(v)
            timestamp = int(ts)

            if "counts." in k or "sets." in k: q.add(key, value, type="counter", measure_time=timestamp, source=self.prefix)
            if "gauges." in k or "kv." in k: q.add(key, value, type="gauge", measure_time=timestamp, source=self.prefix)
            if "timers." in k: q.add(key, value, type="gauge", measure_time=timestamp, display_units_long="ms", source=self.prefix)
            
            q_len += 1    
        
        if q_len > 0:
            for attempt in xrange(self.attempts):
                try:
                    self.logger.info("Outputting ~ %d metrics" % q_len)
                    r = q.submit()
                    self.logger.debug(r)                    
                    return
                except:
                    self.logger.exception("Error while flushing to librato. Reattempting...")                

            self.logger.critical("Failed to flush to Librato! Gave up after %d attempts." % self.attempts)


def main(metrics, user, token, prefix="statsite", attempts=3):
    if metrics == None: return

    # Initialize the logger
    logging.basicConfig()

    # Intialize from our arguments
    sink = LibratoSink(user, token, prefix, attempts)

    # Flush
    sink.flush(metrics.split("\n"))
    sink.close()




if __name__ == "__main__":
    # Get all the inputs
    metrics = sys.stdin.read()

    main(metrics, *sys.argv[1:])

