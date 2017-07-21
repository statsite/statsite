"""
Supports flushing metrics via HTTP
"""
import json
import sys

import requests


class StatsiteHttp(object):
    def __init__(self, url=None):
        """
        Implements an interface that allows metrics to be sent via HTTP
        in the following JSON format:

            {
                "metrics": [
                     {"key": "foo", "value": "123", "timestamp": "1500000000"},
                     {"key": "bar", "value": "456", "timestamp": "1500000001"},
                ]
            }

        Raises a :class:`ValueError` on bad arguments

        :Parameters:
            - `url`: HTTP endpoint to POST metrics to
        """
        if url is None:
            raise ValueError("must provide a URL")

        self._url = url

    def append(self, metric):
        """
        Adds a new metric in the format of `key|value|timestamp`.

        :Parameters:
            - `metric`: metric from statsite
        """
        if metric and metric.count("|") == 2:
            k, v, ts = metric.split("|")
            self.metrics.append({"key": k, "value": v, "timestamp": ts})

    def flush(self):
        """
        Flushes metrics to the defined url

        Raises a :class:`HTTPError` on failure to POST
        """
        if not self.metrics:
            return

        r = requests.post(self._url, data=json.dumps({"metrics": self.metrics}))
        r.raise_for_status()
        self._init_metrics()

    def _init_metrics(self):
        self.metrics = []

    def __enter__(self):
        self._init_metrics()
        return self

    def __exit__(self, *args):
        self.flush()


if __name__ == "__main__":
    with StatsiteHttp(*sys.argv[1:]) as client:
        while True:
            try:
                client.append(raw_input().strip())
            except EOFError:
                break
