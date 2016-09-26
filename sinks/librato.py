"""
Supports flushing statsite metrics to Librato
"""
import ast
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
        self.sink_version = "1.0.1"

        self.flush_timeout_secs = 5
        self.gauges = {}
        self.measurements = {}
        
        # Limit our payload sizes
        self.max_metrics_payload = 500

        self.timer_re = re.compile("^timers\.")
        self.ex_count_re = re.compile("^counts\.")
        self.type_re = re.compile("^(kv|timers|counts|gauges|sets)\.(.+)$")

        self.sfx_map = {
            'sum': 'sum',
            'sum_sq': None,
            'count' : 'count',
            'stdev' : 'stddev_m2',
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
            self.source = None
        
        if config.has_option(sect, 'host'):
            self.host = config.get(sect, 'host')
        else:
            self.host = socket.gethostname()

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
        
        # Check if we want to also send measurements to legacy Librato API
        if config.has_option(sect, "write_to_legacy"):
            self.write_to_legacy = config.getboolean(sect, "write_to_legacy")
        else:
            self.write_to_legacy = False
        
        # Global Tags
        if config.has_option(sect, "tags"):
            self.tags = ast.literal_eval(config.get(sect, "tags"))
        else:
            self.tags = {}

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
    
    def parse_tags(self, name, multipart=False):
        # Find and parse the tags from the name using the syntax name#tag1=value,tag2=value
        s = name.split("#")
        tags = {}
        raw_tags = []
        
        if len(s) > 1:
            name = s.pop(0)
            raw_tags = s.pop().split(",")
            if multipart:
                # Timers will append .p90, .p99 etc to the end of the "metric name"
                # Parse the suffix out and append to the metric name
                last_tag = raw_tags.pop()
                last_tag_split = last_tag.split('.')

                # Store the proper name. The suffix is the last element in split of the last tag.
                name = name + "." + last_tag_split.pop()

                # Put the tag, without the suffix, back in the list of raw tags
                if len(last_tag_split) > 1:
                    t = ".".join(last_tag_split)
                    # # We had periods in the tag value...
                    raw_tags.append(t)
                    # raw_tags.extend(last_tag_split)
                else:
                    raw_tags.extend(last_tag_split)

            # Parse the tags out
            for raw_tag in raw_tags:
                # Get the key and value from tag=value
                tag_key, tag_value = raw_tag.split("=")
                tags[tag_key] = tag_value
        return name, tags
                
        

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

        # Add a source prefix
        if self.source_prefix:
            source = "%s.%s" % (self.source_prefix, source)

        # Parse the tags out
        name, tags = self.parse_tags(name, ismultipart)
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

        name = self.sanitize(name)
        
        # Add the hostname as a global tag. 
        self.tags['host'] = self.host

        if source:
            # Sanitize
            source = self.sanitize(source)

            # Add a tag of source if not specified by the client
            if 'source' not in tags:
                tags['source'] = source

            # Build a key for the dict that will hold all the measurements to
            # submit
            k = "%s\t%s" % (name, source)
        else:
            k = name

        if k not in self.measurements:
            m = [{'name': name, 'tags' : tags, 'time' : ts, subf: value}]
            self.measurements[k] = m
        else:
            # Build summary statistics
            processed = False
            # Try to find an existing measurement for this tagset
            # so we can add the next summary statistic
            for m in self.measurements[k]:
                if m['tags'] == tags:
                    m[subf] = value
                    processed = True
                    break
            if not processed:
                # New tagset
                payload = {'name': name, 'tags' : tags, 'time' : ts, subf: value}
                self.measurements[k].append(payload)

        # Build out the legacy gauges
        if self.write_to_legacy:
            if k not in self.gauges:
                # Truncate metric/source names to 255 for legacy
                if len(name) > 255:
                    name = name[:255]
                    self.logger.warning(
                        "Truncating metric %s to 255 characters to avoid failing entire payload" % name
                    )
                if source and len(source) > 255:
                    source = source[:255]
                    self.logger.warning(
                        "Truncating source %s to 255 characters to avoid failing entire payload" % source
                    )
                self.gauges[k] = {
                    'name': name,
                    'source': source,
                    'measure_time': ts
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

    def flush_payload(self, headers, m, legacy = False):
        """
        POST a payload to Librato.
        """
        
        if legacy:
            body = json.dumps({ 'gauges' : m })
            url = "%s/v1/metrics" % (self.api)
        else:
            global_tags = self.tags
            body = json.dumps({ 'measurements' : m, 'tags': global_tags })
            url = "%s/v1/measurements" % (self.api)
                
        req = urllib2.Request(url, body, headers)

        try:
            f = urllib2.urlopen(req, timeout = self.flush_timeout_secs)
            response = f.read()
            f.close()
            # The new tags API supports partial payload accept/reject
            # So let's show a message if any part fails
            if 'errors' in response:
                parsed_response = json.loads(response)
                # errors could be [], so check that prior to logging anything
                if parsed_response['errors']:
                    self.logger.error(parsed_response)
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
        if len(self.measurements) == 0:
            return
    
        headers = {
            'Content-Type': 'application/json',
            'User-Agent': self.build_user_agent(),
            'Authorization': 'Basic %s' % self.build_basic_auth()
            }

        tagged_metrics = []
        legacy_metrics = []
        count = 0
        
        for v in self.measurements.values():
            for metric in v:
                tagged_metrics.append(metric)
                count += 1

                if count >= self.max_metrics_payload:
                    self.flush_payload(headers, tagged_metrics)
                    count = 0
                    tagged_metrics = []

        if count > 0:
            self.flush_payload(headers, tagged_metrics)
        
        # If enabled, submit flush metrics to Librato's legacy API
        if self.write_to_legacy:
            if len(self.gauges) == 0:
                return
            values = self.gauges.values()
            count = 0
            for measure in values:
                legacy_metrics.append(measure)
                count += 1

                if count >= self.max_metrics_payload:
                    self.flush_payload(headers, legacy_metrics, True)
                    count = 0
                    legacy_metrics = []

            if count > 0:
                self.flush_payload(headers, legacy_metrics, True)
        

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
