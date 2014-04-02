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
# prefix = statsite
# timeout = 10
#
# Options:
# --------
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
        except:            
            self.logger.exception('Failed to send metrics to InfluxDB:', res.status, res.reason)

        conn.close()
    


def main(metrics, *argv):    
    # Initialize the logger
    logging.basicConfig()    

    # Intialize from our arguments
    influxdb = InfluxDBStore(*argv[0:])

    # Flush 
    influxdb.flush(metrics.splitlines())


if __name__ == "__main__":
    # Get all the inputs
    main(sys.stdin.read(), *sys.argv[1:])

