Statsite [![Build Status](https://travis-ci.org/armon/statsite.png)](https://travis-ci.org/armon/statsite)
========

This is a stats aggregation server. Statsite is based heavily
on Etsy's StatsD <https://github.com/etsy/statsd>.

Features
--------

* Basic key/value metrics
* Send timer data, statsite will calculate:
  - Mean
  - Min/Max
  - Standard deviation
  - Median, Percentile 95, Percentile 99
  - Histograms
* Send counters that statsite will aggregate
* Sets for cardinality measurements
* Binary protocol


Architecture
-------------

Statsite is designed to be both highly performant,
and very flexible. To achieve this, it implements the stats
collection and aggregation in pure C, using libev to be
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
and aggregated using the Cormode-Muthurkrishnan algorithm from
"Effective Computation of Biased Quantiles over Data Streams".
This means that the percentile values are not perfectly accurate,
and are subject to a specifiable error epsilon. This allows us to
store only a fraction of the samples.

Histograms can also be optionally maintained for timer values.
The minimum and maximum values along with the bin widths must
be specified in advance, and as samples are recieved the bins
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

Download and build from source::

    $ git clone https://github.com/armon/statsite.git
    $ cd statsite
    $ pip install SCons  # Uses the Scons build system, may not be necessary
    $ scons
    $ ./statsite

Building the test code may generate errors if libcheck is not available.
To build the test code successfully, do the following::

    $ cd deps/check-0.9.8/
    $ ./configure
    $ make
    # make install
    # ldconfig (necessary on some Linux distros)
    $ cd ../../
    $ scons test_runner

At this point, the test code should build successfully.

Usage
-----

Statsite is configured using a simple INI file.
Here is an example configuration file::

    [statsite]
    port = 8125
    udp_port = 8125
    log_level = INFO
    flush_interval = 10
    timer_eps = 0.01
    set_eps = 0.02
    stream_cmd = python sinks/graphite.py localhost 2003

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

Protocol
--------

By default, Statsite will listen for TCP and UDP connections. A message
looks like the following (where the flag is optional)::

    key:value|type[|@flag]

Messages must be terminated by newlines (`\n`).

Currently supported message types:

* `kv` - Simple Key/Value.
* `g`  - Same as `kv`, compatibility with statsd gauges
* `ms` - Timer.
* `c`  - Counter.
* `s`  - Unique Set

After the flush interval, the counters and timers of the same key are
aggregated and this is sent to the store.

Examples:

The following is a simple key/value pair, in this case reporting how many
queries we've seen in the last second on MySQL::

    mysql.queries:1381|kv

The following is a timer, timing the response speed of an API call::

    api.session_created:114|ms

The next example is increments the "rewards" counter by 1::

    rewards:1|c

And this example decrements the "inventory" counter by 7::

    inventory:-7|c

Sets count the unique items, so if statsite gets::

    users:abe|s
    users:zoe|s
    users:bob|s
    users:abe|s

Then it will emit a count 3 for the number of uniques it has seen.

Writing Statsite Sinks
---------------------

Statsite only ships with a graphite sink by default, but any executable
can be used as a sink. The sink should read its inputs from stdin, where
each metric is in the form::

    key|val|timestamp

Each metric is separated by a newline. The process should terminate with
an exit code of 0 to indicate success.


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

* 0x1 : Key value / Gauge
* 0x2 : Counter
* 0x3 : Timer
* 0x4 : Set

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

