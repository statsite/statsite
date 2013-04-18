import socket
import time
import random

NUM = 1024 * 1024
KEYS = ["test", "foobar", "zipzap"]
VALS = [32, 100, 82, 101, 5, 6, 42, 73]

METS = []
for x in xrange(NUM):
    key = random.choice(KEYS)
    val = random.choice(VALS)
    METS.append("%s:%f|c\n" % (key, val))

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

