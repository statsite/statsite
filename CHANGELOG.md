# 0.8.0

* Timer sampling properly handles counts thanks to @nspragg
* Replace libev with the Redis (AE) event loop thanks to @sleepybishop
* Fixed percentile on small data sets picking the upper bound value
* Git SHA: e69cf74

# 0.7.1

* Support for custom quantiles
* Included Cloudwatch sink
* Included OpenTSDB sink
* Fixing minor memory leak
* Support for parsing floating point and exponential formats
* Git SHA: 8c1cfb5

# 0.7.0

* Support for IPv6. Thanks to @ealekseev [GH-57]
* Support for `extended_counters` option to expose more
  summary information about counters.
* Support for Gmetric sink
* Support for Liberato sink
* Fixing issue that would cause STDIN to be closed [GH-74]
* Fixing race condition that would report sink had failed
* Address several thread safety issues
* Configurable syslog facilities
* Support for prefixes on the binary stream
* Git SHA: b29492b

# 0.6.5

* Removed dependency on C++ compiler
* Support for 'h' as an alias of 'ms'
* Support for configurable global_prefix, and type specific
  prefixes thanks to @melkor217
* Git SHA: c004e50

# 0.6.0

* Support streaming input metrics over stdin
* TCP/UDP listeners can be disabled by setting the port to 0
* Increased UDP throughput
* Git SHA: 5169f98

# 0.5.1

* Adding support for delta updates to gauges. An important change in behavior
is that sending multiple negative value gauages will accumulate instead of
using the last value that is set. This is due to the ambiguity between setting
a negative value and applying a delta update. This does not apply to the
binary protocol.
* Improve the graphite sink from @pasku
* Git SHA: 3fe9b9a

# 0.5.0

 * Adding support from gauges. Previously, gauges were handled
 as key/value pairs internally meaning each new gauge would generate
 a corresponding output. Now gauges update the same value, and only the
 last value is retained, which is how statsd behaves.
 * Fix from @dccmx to prevent a SIGPIPE on linux when statsite is daemonized
 * Git SHA: f872039

# 0.4.6

 * Fixed bug with HLL bias correction
 * Fixed bug in exact sets, previously would over count
 * Git SHA: af7d4b6

# 0.4.5

 * Adding support for sets for cardinality estimation
 * Improved command parsing speed dramatically
 * Multiple interrupts will cause statsite to exit, previously
   a bug could cause statsite to hang
 * Git SHA: 8eb1e90

# 0.4.0

 * Adding support for histogram calculations on timers
 * Improved error reporting when parsing config file
 * Git SHA: 1d3dc82

# 0.3.5

 * Cleanup of the networking code, reliability improvements,
   reduced memory footprint.
 * Git SHA: 4eb5e5e

# 0.3.4

 * Compatibility with statsd sampling flag for counters
 * Git SHA: c6002f5e0ba43d7e451be584f24d7f07e890dcf1

# 0.3.3

 * UDP compatibility with statsd clients (allow missing newline).
 thanks to @joeshaw
 * Added ability to count the number of input metrics, and emit
 to a configurable counter, thanks to @joeshaw
 * Git SHA: b28c48be615d2dce5e57a2be56b0117b85e77588

# 0.3.2

 * Fixing critical bug that causes segfault if the first
 timer value is negative.
 * Git SHA: 97e4597aac776aca899482a46f23f72dc25b6e24

# 0.3.1

 * Binary sink protocol support
 * Git SHA: f14bc449487dcb096773c01ed81508b129708ca9

# 0.3.0

 * Binary protocol support
 * Git SHA: a66163b45fbebc807d8caeb9526c43b45be0abc5

# 0.2.1

 * Daemonization support thanks to jgoldschrafe
 * Git SHA: a157350812a4e2fe0cc62ab902ac172b360b4551

# 0.2.0

 * Added UDP support
 * Git SHA: 88e048adada4e9e392a40034c8ad44c36968d0b1

# 0.1.0

 * Initial version released.
 * Git SHA: c3e9fa188b765e8f1fd31e07108af295bb299ee4

