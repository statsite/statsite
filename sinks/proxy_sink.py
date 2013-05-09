"""
Proxies metrics to other sinks
"""
import sys

import graphite
import librato_sink
#
#import other_sink

if __name__ == "__main__":
    # Get all the inputs
    metrics = sys.stdin.read()

    graphite.main(metrics, host="localhost", port=2003, prefix="statsite")
    librato_sink.main(metrics, user="example@librato.com", token="75AFDB82", prefix="statsite")
    #other_sink.main(metrics, **sys.argv[1:])