"""
Supports flushing statsite metrics to InfluxDB
"""
import sys
import httplib, urllib, logging, json, re

##
# InfluxDB sink for statsite
# ==========================
#
# Use with the following stream command:
#
#  stream_command = python sinks/influxdb.py influxdb.ini INFO
#
# The InfluxDB sink takes an INI format configuration file as a first
# argument and log level as a second argument.
# The following is an example configuration:
#
# Configuration example:
# ---------------------
#
# [influxdb]
# host = 127.0.0.1
# port = 8086
# database = dbname
# username = root
# password = root
#
#
# version = 0.9
# prefix = statsite
# timeout = 10
#
# Options:
# --------
#  - version:  InfluxDB version (by default 0.9)
#  - prefix:   A prefix to add to the keys
#  - timeout:  The timeout blocking operations (like connection attempts)
#              will timeout after that many seconds (if it is not given, the global default timeout setting is used)
###

class InfluxDBStore(object):
    def __init__(self, cfg="/etc/statsite/influxdb.ini", lvl="INFO"):
        """
        Implements an interface that allows metrics to be persisted to InfluxDB.

        Raises a :class:`ValueError` on bad arguments or `Exception` on missing
        configuration section.

        :Parameters:
            - `cfg`: INI configuration file.
            - `lvl`: logging level (DEBUG, INFO, WARNING, ERROR, CRITICAL)
        """

        self.sink_name = "statsite-influxdb"
        self.sink_version = "0.0.1"

        self.logger = logging.getLogger("statsite.influxdb")
        self.logger.setLevel(lvl)

        self.load(cfg)


    def load(self, cfg):
        """
        Loads configuration from an INI format file.
        """
        import ConfigParser
        ini = ConfigParser.RawConfigParser()
        ini.read(cfg)

        sect = "influxdb"
        if not ini.has_section(sect):
            raise Exception("Can not locate config section '" + sect + "'")

        if ini.has_option(sect, 'host'):
            self.host = ini.get(sect, 'host')
        else:
            raise ValueError("host must be set in config")

        if ini.has_option(sect, 'port'):
            self.port = ini.get(sect, 'port')
        else:
            raise ValueError("port must be set in config")

        if ini.has_option(sect, 'database'):
            self.database = ini.get(sect, 'database')
        else:
            raise ValueError("database must be set in config")

        if ini.has_option(sect, 'username'):
            self.username = ini.get(sect, 'username')
        else:
            raise ValueError("username must be set in config")

        if ini.has_option(sect, 'password'):
            self.password = ini.get(sect, 'password')
        else:
            raise ValueError("password must be set in config")

        self.prefix = None
        if ini.has_option(sect, 'prefix'):
            self.prefix = ini.get(sect, 'prefix')

        self.timeout = None
        if ini.has_option(sect, 'timeout'):
            self.timeout = ini.get(sect, 'timeout')

        self.version = '0.9'
        if ini.has_option(sect, 'version'):
            self.version = ini.get(sect, 'version')


    def flush09(self, metrics):
        """
        Flushes the metrics provided to InfluxDB v0.9+.

        Parameters:
        - `metrics` : A list of (key,value,timestamp) tuples.
        """
        if not metrics:
            return

        if self.timeout:
            conn = httplib.HTTPConnection(self.host, int(self.port), timeout = int(self.timeout))
        else:
            conn = httplib.HTTPConnection(self.host, int(self.port))

        params = urllib.urlencode({'db': self.database, 'u': self.username, 'p': self.password, 'precision':'s'})
        headers = {
            'Content-Type': 'application/stream',
            'User-Agent': 'statsite',
        }

        # Construct the output
        metrics = [m.split("|") for m in metrics if m]
        self.logger.info("Outputting %d metrics" % len(metrics))

        # Serialize and send via HTTP API
        # InfluxDB uses following regexp "^[a-zA-Z][a-zA-Z0-9._-]*$" to validate table/series names,
        # so disallowed characters replace by '.'
        body = ''
        for k, v, ts in metrics:
            if self.prefix:
                body += "%s" % re.sub(r'[^a-zA-Z0-9.,=_-]+','.', "%s.%s" % (self.prefix, k))
            else:
                body += "%s" % re.sub(r'[^a-zA-Z0-9.,=_-]+','.', k)

            body += " value=" + v + " " + ts + "\n"

        self.logger.debug(body)
        conn.request("POST", "/write?%s" % params, body, headers)
        try:
            res = conn.getresponse()
            info = "%s, %s" % (res.status, res.reason)
            if (200 == res.status):
                self.logger.warning("%s: InfluxDB understood the request but could not complete it: %s" % (info, res.read()))
            elif (204 == res.status):
                self.logger.info("%s: Success!" % info)
            elif (res.status >= 400 and res.status < 500):
                self.logger.error("%s: InfluxDB could not understand the request: %s" % (info, res.read()))
            elif (res.status >= 500 and res.status < 600):
                self.logger.error("%s: The system is overloaded or significantly impaired: %s" % (info, res.read()))
            else:
                self.logger.info(info)

        except Exception:
            self.logger.exception('Failed to send metrics to InfluxDB:', res.status, res.reason)

        conn.close()

    def flush(self, metrics):
        """
        Flushes the metrics provided to InfluxDB.

        Parameters:
        - `metrics` : A list of (key,value,timestamp) tuples.
        """
        if not metrics:
            return

        if self.timeout:
            conn = httplib.HTTPConnection(self.host, int(self.port), timeout = int(self.timeout))
        else:
            conn = httplib.HTTPConnection(self.host, int(self.port))

        params = urllib.urlencode({'u': self.username, 'p': self.password, 'time_precision':'s'})
        headers = {
            'Content-Type': 'application/json',
            'User-Agent': 'statsite',
        }

        # Construct the output
        metrics = [m.split("|") for m in metrics if m]
        self.logger.info("Outputting %d metrics" % len(metrics))

        # Serialize to JSON and send via HTTP API
        # InfluxDB uses following regexp "^[a-zA-Z][a-zA-Z0-9._-]*$" to validate table/series names,
        # so disallowed characters replace by '.'
        if self.prefix:
            body = json.dumps([{
                                    "name":"%s" % re.sub(r'[^a-zA-Z0-9._-]+','.', "%s.%s" % (self.prefix, k)),
                                    "columns":["value", "time"],
                                    "points":[[float(v), int(ts)]]
                                }   for k, v, ts in metrics])
        else:
            body = json.dumps([{
                                    "name":"%s" % re.sub(r'[^a-zA-Z0-9._-]+','.', k),
                                    "columns":["value", "time"],
                                    "points":[[float(v), int(ts)]]
                                }   for k, v, ts in metrics])

        self.logger.debug(body)
        conn.request("POST", "/db/%s/series?%s" % (self.database, params), body, headers)
        try:
            res = conn.getresponse()
            self.logger.info("%s, %s" %(res.status, res.reason))
        except Exception:
            self.logger.exception('Failed to send metrics to InfluxDB:', res.status, res.reason)

        conn.close()


def version(v):
    parts = [int(x) for x in v.split(".")]
    while parts[-1] == 0:
        parts.pop()
    return parts


def main(metrics, *argv):
    # Initialize the logger
    logging.basicConfig()

    # Intialize from our arguments
    influxdb = InfluxDBStore(*argv[0:])

    # Flush format depends on InfluxDB version
    if cmp(version(influxdb.version), version('0.9')) < 0:
        influxdb.flush(metrics.splitlines())
    else:
        influxdb.flush09(metrics.splitlines())


if __name__ == "__main__":
    # Get all the inputs
    main(sys.stdin.read(), *sys.argv[1:])
