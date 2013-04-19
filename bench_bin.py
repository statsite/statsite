import socket
import time
import random
import struct

NUM = 1024 * 1024
KEYS = ["test", "foobar", "zipzap"]
VALS = [32, 100, 82, 101, 5, 6, 42, 73]

BINARY_HEADER = struct.Struct("<BBHd")
BINARY_SET_HEADER = struct.Struct("<BBHH")
BIN_TYPES = {"kv": 1, "c": 2, "ms": 3, "set": 4}


def format(key, type, val):
    "Formats a binary message for statsite"
    key = str(key)
    key_len = len(key) + 1
    type_num = BIN_TYPES[type]
    header = BINARY_HEADER.pack(170, type_num, key_len, float(val))
    mesg = header + key + "\0"
    return mesg


def format_set(key, val):
    "Formats a binary set message for statsite"
    key = str(key)
    key_len = len(key) + 1
    val = str(val)
    val_len = len(val) + 1

    type_num = BIN_TYPES["set"]
    header = BINARY_SET_HEADER.pack(170, type_num, key_len, val_len)
    mesg = "".join([header, key, "\0", val, "\0"])
    return mesg


METS = []
for x in xrange(NUM):
    key = random.choice(KEYS)
    val = str(x) #random.choice(VALS)
    METS.append(format_set(key, val))

s = socket.socket()
s.connect(("localhost", 8125))
start = time.time()

total = 0
while True:
    current = 0
    while current < len(METS):
        msg = "".join(METS[current:current + 1024])
        current += 1024
        total += 1024
        s.sendall(msg)

    diff = time.time() - start
    ops_s = total / diff
    print "%0.2f sec\t - %.0f ops/sec" % (diff, ops_s)

