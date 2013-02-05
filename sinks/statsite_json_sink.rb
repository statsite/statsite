#!/usr/bin/env ruby
# encoding: utf-8

# Usage
#
# statsite.ini:
#   binary_stream = yes
#   stream_cmd = ruby1.9.3 statsite_json_sink.rb -o /dev/shm/statsite-sink.json -f json
#
# available options:
#   -o => output file name (default: /dev/shm/statsite-sink.json)
#   -f => serialization format - one of json, yaml or marshal (default: json)
#   -d => enable debug output (default: false)
#
# notes:
#   - this sink will not automatically convert between serialization formats

TYPES = {
  1 => 'kv',
  2 => 'counter',
  3 => 'timer'
}

VTYPES = {
  0 => 'kv',
  1 => 'sum',
  2 => 'sumsqrt',
  3 => 'mean',
  4 => 'count',
  5 => 'stddev',
  6 => 'min',
  7 => 'max',
  128 => 'percentile'
}

$debug = false

# Calculate percentiles
(1..100).each { |n| VTYPES[128|n] = "P%02d" % n }


# An iterator over a statsite binary stream
# Yields: key, metric type, value, value type, timestamp
def rawdata_iter(fh)
  # timestamp | type  | value type | key length | value  | key
  # ----------+-------+------------+------------+--------+-----
  #   uint64  | uint8 |   uint8    |   uint16   | double |
  Enumerator.new do |enum|
    while true
      prefix = fh.read(20)
      break if prefix.nil? || prefix.length != 20
      ts, type, vtype, key_len, val = prefix.unpack('QCCSD')
      key = fh.read(key_len).chop().force_encoding('utf-8')

      enum.yield key, type, val, vtype, ts
    end
  end
end


# Create a hash from a rawdata_iter.
# { 'metric-name' => {
#      'sum'=>val, 'ts'=>val, 'type'=>val,
#      'sumsqrt'=>val, 'mean'=>val, 'count'=>val,
#      'stddev'=>val, 'min'=>val, 'max'=>val }
# }
def collect(source, resolve=true)
  data = Hash.new {|h,k| h[k] = {}}
  source.each do |key, type, val, vtype, ts|
    puts "read: #{ts}: #{key}|#{type} #{val}|#{vtype}" if $debug
    type,vtype = TYPES[type], VTYPES[vtype] if resolve
    data[key][vtype] = val
    data[key]['ts'] = ts
    data[key]['type'] = type
  end
  data
end

def min(*args) args.min end
def max(*args) args.max end

# merge the previously and currently collected hashes
# counters and aggregated
def merge(old, new)
  old.update(new) do |key, oldv, newv|
    if (oldv['type'] == 'counter') and (newv['type'] == 'counter')
      %w{sum count sumsqrt}.each{ |i| newv[i] += oldv[i] }
      newv['min'] = min oldv['min'], newv['min']
      newv['max'] = max oldv['max'], newv['max']
      newv
    else newv end
  end
end

def load_data(pth, format='json')
  File.open(pth) do |fh|
    case format
    when 'json'
      require 'json'
      JSON.load(fh)
    when 'yaml'
      require 'yaml'
      YAML.load(fh)
    when 'marshal'
      Marshal.load(fh)
    end
  end
end

def dump_data(pth, data, format='json')
  File.open(pth, 'w') do |fh|
    case format
    when 'json'
      require 'json'
      fh.write JSON.pretty_generate(data)
    when 'yaml'
      require 'yaml'
      YAML.dump(data, fh)
    when 'marshal'
      Marshal.dump(data, fh)
    end
  end
end

def parseopts
  # defaults
  dest = '/dev/shm/statsite-sink.json'
  debug = false
  format = 'json'

  loop {
    case ARGV[0]
    when '-o', '--output' then ARGV.shift; dest = ARGV.shift
    when '-f', '--format' then ARGV.shift; format = ARGV.shift
    when '-d', '--debug'  then ARGV.shift; debug = true
    else break
    end
  }

  formats = %w{json yaml marshal}
  abort "unsupported format: #{format}" if !formats.include? format

  return dest, debug, format
end

def main
  dest, $debug, format = parseopts
  old = File.exists?(dest) ? load_data(dest, format) : {}
  new = collect rawdata_iter($stdin.binmode())
  res = merge(old, new)
  dump_data(dest, res, format)
end


main() if __FILE__ == $0
