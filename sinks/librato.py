"""
Supports flushing statsite metrics to Librato
"""
import sys
import socket
import logging
import ConfigParser
import re
import base64
import urllib2
import json
import os

##
# Librato sink for statsite
# =========================
#
# Use with the following stream command:
#
#  stream_cmd = python sinks/librato.py librato.ini
#
# The Librato sink takes an INI format configuration file as a single
# argument. The following is an example configuration:
#
# Configuration example:
# ---------------------
#
# [librato]
# email = john@example.com
# token = 02ac4003c4fcd11bf9cee34e34263155dc7ba1906c322d167db6ab4b2cd2082b
# source_regex = ^([^-]+)--
# floor_time_secs = 60
#
# Options:
# -------
#
#  - email / token: Librato account credentials (required).
#  - source: Source name to use for samples, defaults to hostname if not set.
#  - source_regex: Source name regex extraction see:
#                  https://github.com/librato/statsd-librato-backend#setting-the-source-per-metric
#  - floor_time_secs: Floor samples to this time (should match statsite flush_interval.
#  - prefix: Metric name prefix to set for all metrics.
#  - extended_counters: true/false, look for statsite extended_counters, default false.
#                       This should match your statsite config for extended_counters.
#
###

class LibratoStore(object):
    def __init__(self, conffile="/etc/statsite/librato.ini"):
        """
        Implements an interface that allows metrics to be persisted to Librato.

        Raises a :class:`ValueError` on bad arguments or `Exception` on missing
        configuration section.

        :Parameters:
            - `conffile`: INI configuration file.
        """

        self.logger = logging.getLogger("statsite.librato")

        self.api = "https://metrics-api.librato.com"
        self.parse_conf(conffile)

        self.sink_name = "statsite-librato"
        self.sink_version = "0.0.1"

        self.flush_timeout_secs = 5
        self.gauges = {}

        # Limit our payload sizes
        self.max_metrics_payload = 500

        self.timer_re = re.compile("^timers\.")
        self.ex_count_re = re.compile("^counts\.")
        self.type_re = re.compile("^(kv|timers|counts|gauges|sets)\.(.+)$")

        self.sfx_map = {
            'sum': 'sum',
            'sum_sq' : 'sum_squares',
            'count' : 'count',
            'stdev' : None,
            'lower' : 'min',
            'upper' : 'max',
            'mean' : None
            }
        self.sfx_re = re.compile("(.+)\.(sum|sum_sq|count|stdev|lower|upper|mean)$")
        self.sanitize_re = re.compile("[^-A-Za-z0-9.:_]")

    def parse_conf(self, conffile):
        """
        Loads configuration from an INI format file.
        """

        sect = "librato"

        config = ConfigParser.RawConfigParser()
        config.read(conffile)

        if not config.has_section(sect):
            raise Exception("Can not locate config section 'librato'")

        if config.has_option(sect, 'email'):
            self.email = config.get(sect, 'email')
        else:
            raise ValueError("email must be set in config")

        if config.has_option(sect, 'token'):
            self.token = config.get(sect, 'token')
        else:
            raise ValueError("token must be set in config")

        if config.has_option(sect, 'api'):
            self.api = config.get(sect, 'api')

        if config.has_option(sect, 'source'):
            self.source = config.get(sect, 'source')
        else:
            self.source = socket.gethostname()

        if config.has_option(sect, 'source_regex'):
            reg = config.get(sect, 'source_regex')
            # Strip /'s
            if len(reg) > 2 and reg[0] == '/' and \
               reg[len(reg) - 1] == "/":
                reg = reg[1:len(reg)-1]

            self.source_re = re.compile(reg)
        else:
            self.source_re = None

        if config.has_option(sect, 'floor_time_secs'):
            self.floor_time_secs = config.getint(sect, 'floor_time_secs')
        else:
            self.floor_time_secs = None

        if config.has_option(sect, "prefix"):
            self.prefix = config.get(sect, "prefix")
        else:
            self.prefix = None

        if config.has_option(sect, "source_prefix"):
            self.source_prefix = config.get(sect, "source_prefix")
        else:
            self.source_prefix = None

        if config.has_option(sect, "extended_counters"):
            self.extended_counters = config.getboolean(sect, "extended_counters")
        else:
            self.extended_counters = False

    def split_multipart_metric(self, name):
        m = self.sfx_re.match(name)
        if m != None:
            if self.sfx_map[m.group(2)] != None:
                return m.group(1), self.sfx_map[m.group(2)]
            else:
                # These we drop
                return None, None
        else:
            return name, None

    def sanitize(self, name):
        return self.sanitize_re.sub("_", name)

    def add_measure(self, key, value, time):
        ts = int(time)
        if self.floor_time_secs != None:
            ts = (ts / self.floor_time_secs) * self.floor_time_secs

        value = float(value)
        source = self.source
        name = self.type_re.match(key).group(2)

        ismultipart = False
        if self.timer_re.match(key) != None:
            ismultipart = True
        if self.extended_counters and \
           self.ex_count_re.match(key) != None:
            ismultipart = True

        # Match the source regex
        if self.source_re != None:
            m = self.source_re.search(name)
            if m != None:
                source = m.group(1)
                name = name[0:m.start(0)] + name[m.end(0):]

        subf = None
        if ismultipart:
            name, subf = self.split_multipart_metric(name)
        if subf == None:
            subf = 'value'

        # Bail if skipping
        if name == None:
            return

        # Add a metric prefix
        if self.prefix:
            name = "%s.%s" % (self.prefix, name)

        # Add a source prefix
        if self.source_prefix:
            source = "%s.%s" % (self.source_prefix, source)

        name = self.sanitize(name)
        source = self.sanitize(source)

        k = "%s\t%s" % (name, source)
        if k not in self.gauges:
            self.gauges[k] = {
                'name' : name,
                'source' : source,
                'measure_time' : ts,
                }

        self.gauges[k][subf] = value

    def build(self, metrics):
        """
        Build metric data to send to Librato

       :Parameters:
        - `metrics` : A list of (key,value,timestamp) tuples.
        """
        if not metrics:
            return

        # Construct the output
        for m in metrics:
            k, vs, ts = m.split("|")

            self.add_measure(k, vs, ts)

    def flush_payload(self, headers, g):
        """
        POST a payload to Librato.
        """

        body = json.dumps({ 'gauges' : g })

        url = "%s/v1/metrics" % (self.api)
        req = urllib2.Request(url, body, headers)

        try:
            f = urllib2.urlopen(req, timeout = self.flush_timeout_secs)
            response = f.read()
            f.close()
        except urllib2.HTTPError as error:
            body = error.read()
            self.logger.warning('Failed to send metrics to Librato: Code: %d. Response: %s' % \
                                (error.code, body))
        except IOError as error:
            if hasattr(error, 'reason'):
                self.logger.warning('Error when sending metrics Librato (%s)' % (error.reason))
            elif hasattr(error, 'code'):
                self.logger.warning('Error when sending metrics Librato (%s)' % (error.code))
            else:
                self.logger.warning('Error when sending metrics Librato and I dunno why')

    def flush(self):
        """
        POST a collection of gauges to Librato.
        """

        # Nothing to do
        if len(self.gauges) == 0:
            return

        headers = {
            'Content-Type': 'application/json',
            'User-Agent': self.build_user_agent(),
            'Authorization': 'Basic %s' % self.build_basic_auth()
            }

        metrics = []
        count = 0
        for g in self.gauges.values():
            metrics.append(g)
            count += 1

            if count >= self.max_metrics_payload:
                self.flush_payload(headers, metrics)
                count = 0
                metrics = []

        if count > 0:
            self.flush_payload(headers, metrics)

    def build_basic_auth(self):
        base64string = base64.encodestring('%s:%s' % (self.email, self.token))
        return base64string.translate(None, '\n')

    def build_user_agent(self):
        try:
            uname = os.uname()
            system = "; ".join([uname[0], uname[4]])
        except Exception:
            system = os.name()

        pver = sys.version_info
        user_agent = '%s/%s (%s) Python-Urllib2/%d.%d' % \
                     (self.sink_name, self.sink_version, system, pver[0], pver[1])
        return user_agent

if __name__ == "__main__":
    # Initialize the logger
    logging.basicConfig()

    # Intialize from our arguments
    librato = LibratoStore(*sys.argv[1:])

    # Get all the inputs
    metrics = sys.stdin.read()

    # Flush
    librato.build(metrics.splitlines())
    librato.flush()
