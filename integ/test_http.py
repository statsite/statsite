"""
Testing the http sink
"""
import json
from mock import patch

from sinks.http import StatsiteHttp


@patch('requests.post')
def test_http(req):
    metrics = [
        "foo|123|1500000000",
        "bar|456|1500000001",
    ]

    expected = [
        {"key": "foo", "value": "123", "timestamp": "1500000000"},
        {"key": "bar", "value": "456", "timestamp": "1500000001"},
    ]

    url = 'https://url.com'
    sink = StatsiteHttp(url)
    with sink:
        for metric in metrics:
            sink.append(metric)
        assert expected == sink.metrics

    req.assert_called_with(url, data=json.dumps({"metrics": expected}))
    assert sink.metrics == []
