Statsite [![Build Status](https://travis-ci.org/armon/statsite.png)](https://travis-ci.org/armon/statsite)
========

Statsite is a metrics aggregation server. Statsite is based heavily
on Etsy's StatsD <https://github.com/etsy/statsd>, and is wire compatible.

Features
--------

* Multiple metric types
  - Key / Value
  - Gauges
  - Counters
  - Timers
  - Sets
* Efficient summary metrics for timer data:
  - Mean
  - Min/Max
  - Standard deviation
  - Median, Percentile 95, Percentile 99
  - Histograms
* Dynamic set implementation:
  - Exactly counts for small sets
  - HyperLogLog for large sets
* Included sinks:
  - Graphite
  - InfluxDB
  - Ganglia
  - Librato
  - CloudWatch
  - OpenTSDB
* Binary protocol
* TCP, UDP, and STDIN
* Fast


Architecture
-------------

Statsite is designed to be both highly performant,
and very flexible. To achieve this, it implements the stats
collection and aggregation in pure C, using an event loop to be
extremely fast. This allows it to handle hundreds of connections,
and millions of metrics. After each flush interval expires,
statsite performs a fork/exec to start a new stream handler
invoking a specified application. Statsite then streams the
aggregated metrics over stdin to the application, which is
free to handle the metrics as it sees fit.

This allows statsite to aggregate metrics and then ship metrics
to any number of sinks (Graphite, SQL databases, etc). There
is an included Python script that ships metrics to graphite.

Statsite tries to minimize memory usage by not
storing all the metrics that are received. Counter values are
aggregated as they are received, and timer values are stored
and aggregated using the Cormode-Muthukrishnan algorithm from
"Effective Computation of Biased Quantiles over Data Streams".
This means that the percentile values are not perfectly accurate,
and are subject to a specifiable error epsilon. This allows us to
store only a fraction of the samples.

Histograms can also be optionally maintained for timer values.
The minimum and maximum values along with the bin widths must
be specified in advance, and as samples are received the bins
are updated. Statsite supports multiple histograms configurations,
and uses a longest-prefix match policy.

Handling of Sets in statsite depend on the number of
entries received. For small cardinalities (<64 currently),
statsite will count exactly the number of unique items. For
larger sets, it switches to using a HyperLogLog to estimate
cardinalities with high accuracy and low space utilization.
This allows statsite to estimate huge set sizes without
retaining all the values. The parameters of the HyperLogLog
can be tuned to provide greater accuracy at the cost of memory.

The HyperLogLog is based on the Google paper, "HyperLogLog in
Practice: Algorithmic Engineering of a State of The Art Cardinality
Estimation Algorithm".

Install
-------

Download and build from source. This requires `autoconf`, `automake` and `libtool` to be available,
available usually through a system package manager. Steps:

    $ git clone https://github.com/armon/statsite.git
    $ cd statsite
    $ ./bootstrap.sh
    $ ./configure
    $ make
    $ ./src/statsite

Building the test code may generate errors if libcheck is not available.
To build the test code successfully, do the following::

    $ cd deps/check-0.9.8/
    $ ./configure
    $ make
    # make install
    # ldconfig (necessary on some Linux distros)
    $ cd ../../
    $ make test

At this point, the test code should build successfully.

Usage
-----

Statsite is configured using a simple INI file.
Here is an example configuration file::

    [statsite]
    port = 8125
    udp_port = 8125
    log_level = INFO
    log_facility = local0
    flush_interval = 10
    timer_eps = 0.01
    set_eps = 0.02
    stream_cmd = python sinks/graphite.py localhost 2003 statsite

    [histogram_api]
    prefix=api
    min=0
    max=100
    width=5

    [histogram_default]
    prefix=
    min=0
    max=200
    width=20

Then run statsite, pointing it to that file::

    statsite -f /etc/statsite.conf

A full list of configuration options is below.

Configuration Options
---------------------

Each statsite configuration option is documented below. Statsite configuration
options must exist in the `statsite` section of the INI file:

* tcp\_port : Integer, sets the TCP port to listen on. Default 8125. 0 to disable.

* port: Same as above. For compatibility.

* udp\_port : Integer, sets the UDP port. Default 8125. 0 to disable.

* bind\_address : The address to bind on. Defaults to 0.0.0.0

* parse\_stdin: Enables parsing stdin as an input stream. Defaults to 0.

* log\_level : The logging level that statsite should use. One of:
  DEBUG, INFO, WARN, ERROR, or CRITICAL. All logs go to syslog,
  and also stderr when not daemonizing. Default is DEBUG.

* log\_facility : The syslog logging facility that statsite should use.
  One of: user, daemon, local0, local1, local2, local3, local4, local5,
  local6, local7. All logs go to syslog.

* flush\_interval : How often the metrics should be flushed to the
  sink in seconds. Defaults to 10 seconds.

* timer\_eps : The upper bound on error for timer estimates. Defaults
  to 1%. Decreasing this value causes more memory utilization per timer.

* set\_eps : The upper bound on error for unique set estimates. Defaults
  to 2%. Decreasing this value causes more memory utilization per set.

* stream\_cmd : This is the command that statsite invokes every
  `flush_interval` seconds to handle the metrics. It can be any executable.
  It should read inputs over stdin and exit with status code 0 on success.

* input\_counter : If set, statsite will count how many commands it received
  in the flush interval, and the count will be emitted under this name. For
  example if set to "numStats", then statsite will emit "counter.numStats" with
  the number of samples it has received.

* daemonize : Should statsite daemonize. Defaults to 0.

* pid\_file : When daemonizing, where to put the pid file. Defaults
  to /var/run/statsite.pid

* binary\_stream : Should data be streamed to the stream\_cmd in
  binary form instead of ASCII form. Defaults to 0.

* use\_type\_prefix : Should prefixes with message type be added to the messages.
  Does not affect global\_prefix. Defaults to 1.

* global\_prefix : Prefix that will be added to all messages.
  Defaults to empty string.

* kv\_prefix, gauges\_prefix, counts\_prefix, sets\_prefix, timers\_prefix : prefix for
  each message type. Defaults to respectively: "kv.", "gauges.", "counts.",
  "sets.", "timers.". Values will be ignored if use_type_prefix set to 0.

* extended\_counters : If enabled, the counter output is extended to include
  all the computed summary values. Otherwise, the counter is emitted as just
  the sum value. Summary values include `count`, `mean`, `stdev`, `sum`, `sum_sq`,
  `lower`, `upper`, and `rate`.
  Defaults to false.

* extended\_counters\_include : Allows you to configure which extended counters to include
  through a comma separated list of values, extended\_counters must be set to true. Supported values include `count`, `mean`, `stdev`, `sum`, `sum_sq`,
  `lower`, `upper`, and `rate`. If this option is not specified but extended_counters is set to true, then all values will be included by default.

* timers\_include : Allows you to configure which timer metrics to include
  through a comma separated list of values. Supported values include `count`, `mean`, `stdev`, `sum`, `sum_sq`,
  `lower`, `upper`, `rate`, `median` and `sample_rate`. If this option is not specified then all values except `median` will be included by default.
  `median` will be included if `quantiles` include 0.5

* prefix\_binary\_stream : If enabled, the keys streamed to a the stream\_cmd
  when using binary\_stream mode are also prefixed. By default, this is false,
  and keys do not get the prefix.

* quantiles : A comma-separated list of quantiles to calculate for timers.
  Defaults to `0.5, 0.95, 0.99`

In addition to global configurations, statsite supports histograms
as well. Histograms are configured one per section, and the INI
section must start with the word `histogram`. These are the recognized
options:

* prefix : This is the key prefix to match on. The longest matching prefix
  is used. If the prefix is blank, it is the default for all keys.

* min : Floating value. The minimum bound on the histogram. Values below
  this go into a special bucket containing everything less than this value.

* max: Floating value. The maximum bound on the histogram. Values above
  this go into a special bucket containing everything more than this value.

* width : Floating value. The width of each bucket between the min and max.

Each histogram section must specify all options to be valid.


Protocol
--------

By default, Statsite will listen for TCP and UDP connections. A message
looks like the following (where the flag is optional)::

    key:value|type[|@flag]

Messages must be terminated by newlines (`\n`).

Currently supported message types:

* `kv` - Simple Key/Value.
* `g`  - Gauge, similar to `kv` but only the last value per key is retained
* `ms` - Timer.
* `h`  - Alias for timer
* `c`  - Counter.
* `s`  - Unique Set

After the flush interval, the counters and timers of the same key are
aggregated and this is sent to the store.

Gauges also support "delta" updates, which are supported by prefixing the
value with either a `+` or a `-`. This implies you can't explicitly set a gauge to a negative number without first setting it to zero.

Examples:

The following is a simple key/value pair, in this case reporting how many
queries we've seen in the last second on MySQL::

    mysql.queries:1381|kv

The following is a timer, timing the response speed of an API call::

    api.session_created:114|ms

The next example increments the "rewards" counter by 1::

    rewards:1|c

Here we initialize a gauge and then modify its value::

    inventory:100|g
    inventory:-5|g
    inventory:+2|g

Sets count the unique items, so if statsite gets::

    users:abe|s
    users:zoe|s
    users:bob|s
    users:abe|s

Then it will emit a count 3 for the number of uniques it has seen.

Writing Statsite Sinks
---------------------

Statsite ships with graphite, librato, gmetric, and influxdb sinks, but ANY executable
or script  can be used as a sink. The sink should read its inputs from stdin, where
each metric is in the form::

    key|val|timestamp\n

Each metric is separated by a newline. The process should terminate with
an exit code of 0 to indicate success.

Here is an example of the simplest possible Python sink:

    #!/usr/bin/env python
    import sys

    lines = sys.stdin.read().split("\n")
    metrics = [l.split("|") for l in lines]

    for key, value, timestamp in metrics:
        print key, value, timestamp


Binary Protocol
---------------

In addition to the statsd compatible ASCII protocol, statsite includes
a lightweight binary protocol. This can be used if you want to make use
of special characters such as the colon, pipe character, or newlines. It
is also marginally faster to process, and may provide 10-20% more throughput.

Each command is sent to statsite over the same ports with this header:

    <Magic Byte><Metric Type><Key Length>

Then depending on the metric type, it is followed by either:

    <Value><Key>
    <Set Length><Key><Set Key>

The "Magic Byte" is the value 0xaa (170). This switches the internal
processing from the ASCII mode to binary. The metric type is one of:

* 0x1 : Key value / Gauge
* 0x2 : Counter
* 0x3 : Timer
* 0x4 : Set
* 0x5 : Gauge
* 0x6 : Gauge Delta update

The key length is a 2 byte unsigned integer with the length of the
key, INCLUDING a NULL terminator. The key must include a null terminator,
and it's length must include this.

If the metric type is K/V, Counter or Timer, then we expect a value and
a key. The value is a standard IEEE754 double value, which is 8 bytes in length.
The key is provided as a byte stream which is `Key Length` long,
terminated by a NULL (0) byte.

If the metric type is Set, then we expect the length of a set key,
provided like the key length. The key should then be followed by
an additional Set Key, which is `Set Length` long, terminated
by a NULL (0) byte.

All of these values must be transmitted in Little Endian order.

Here is an example of sending ("Conns", "c", 200) as hex:

    0xaa 0x02 0x0600 0x0000000000006940 0x436f6e6e7300


Note: The binary protocol does not include support for "flags" and resultantly
cannot be used for transmitting sampled counters.


Binary Sink Protocol
--------------------

It is also possible to have the data streamed to be represented
in a binary format. Again, this is used if you want to use the reserved
characters. It may also be faster.

Each command is sent to the sink in the following manner:

    <Timestamp><Metric Type><Value Type><Key Length><Value><Key>[<Count>]

Most of these are the same as the binary protocol. There are a few.
changes however. The Timestamp is sent as an 8 byte unsigned integer,
which is the current Unix timestamp. The Metric type is one of:

* 0x1 : Key value
* 0x2 : Counter
* 0x3 : Timer
* 0x4 : Set
* 0x5 : Gauge

The value type is one of:

* 0x0 : No type (Key/Value)
* 0x1 : Sum (Also used for Sets)
* 0x2 : Sum Squared
* 0x3 : Mean
* 0x4 : Count
* 0x5 : Standard deviation
* 0x6 : Minimum Value
* 0x7 : Maximum Value
* 0x8 : Histogram Floor Value
* 0x9 : Histogram Bin Value
* 0xa : Histogram Ceiling Value
* 0xb : Count Rate (Sum / Flush Interval)
* 0xc : Sample Rate (Count / Flush Interval)
* 0x80 OR `percentile` :  If the type OR's with 128 (0x80), then it is a
    percentile amount. The amount is OR'd with 0x80 to provide the type. For
    example (0x80 | 0x32) = 0xb2 is the 50% percentile or medium. The 95th
    percentile is (0x80 | 0xdf) = 0xdf.

The key length is a 2 byte unsigned integer representing the key length
terminated by a NULL character. The Value is an IEEE754 double. Lastly,
the key is a NULL-terminated character stream.

The final `<Count>` field is only set for histogram values.
It is always provided as an unsigned 32 bit integer value. Histograms use the
value field to specify the bin, and the count field for the entries in that
bin. The special values for histogram floor and ceiling indicate values that
were outside the specified histogram range. For example, if the min value was
50 and the max 200, then HISTOGRAM\_FLOOR will have value 50, and the count is
the number of entires which were below this minimum value. The ceiling is the same
but visa versa. For bin values, the value is the minimum value of the bin, up to
but not including the next bin.

To enable the binary sink protocol, add a configuration variable `binary_stream`
to the configuration file with the value `yes`. An example sink is provided in
`sinks/binary_sink.py`.

