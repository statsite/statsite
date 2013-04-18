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
    METS.append("%s:%f|c|@123\n" % (key, val))

s = socket.socket()
s.connect(("localhost", 8125))
start = time.time()

current = 0
while current < len(METS):
    msg = "".join(METS[current:current + 1024])
    current += 1024
    s.sendall(msg)

s.close()
end = time.time()
print NUM / (end - start), "ops/sec", (end - start), "sec"

