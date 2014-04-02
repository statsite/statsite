"""
Proxies metrics to other sinks
"""
import sys

import graphite
import influxdb

if __name__ == "__main__":
    # Get all the inputs
    metrics = sys.stdin.read()

    graphite.main(metrics, host="localhost", port=2003, prefix="statsite")    
    influxdb.main(metrics, "/etc/statsite/influxdb.ini")

