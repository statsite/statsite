import struct
import sys

# Line format. We have:
# 8 byte unsigned timestamp
# 1 byte metric type
# 1 byte value type
# 2 byte key length
# 8 byte value
LINE = struct.Struct("<QBBHd")
COUNTER = struct.Struct("<I")
PREFIX_SIZE = 20

TYPE_MAP = {
    1: "kv",
    2: "counter",
    3: "timer",
    4: "set",
    5: "gauge"
}
VAL_TYPE_MAP = {
    0: "kv",
    1: "sum",
    2: "sum sq",
    3: "mean",
    4: "count",
    5: "stddev",
    6: "min",
    7: "max",
    8: "hist_min",
    9: "hist_bin",
    10: "hist_max",
    128: "percentile"
}
# Pre-compute all the possible percentiles
for x in xrange(1, 100):
    VAL_TYPE_MAP[128 | x] = "P%02d" % x


def main():
    while True:
        # Read the prefix
        prefix = sys.stdin.read(20)
        if not prefix or len(prefix) != 20:
            return

        # Unpack the line
        (ts, type, val_type, key_len, val) = LINE.unpack(prefix)
        type = TYPE_MAP[type]
        val_type = VAL_TYPE_MAP[val_type]

        # Read the key
        key = sys.stdin.read(key_len)

        if val_type == "stddev":
            print repr(prefix)

        # Print
        if val_type.startswith("hist"):
            count = COUNTER.unpack(sys.stdin.read(4))
            print ts, type, val_type, key, val, count

        else:
            print ts, type, val_type, key, val


if __name__ == "__main__":
    main()

