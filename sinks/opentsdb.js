var net = require('net');


/*
    Put into your statsite config as:
    ****
    stream_cmd = node sinks/opentsdb.js 127.0.0.1 4242 _t_
    ****
*/


if (process.argv.length != 5)
{
    console.log('Wrong number of arguments');
    console.log('opentsdbsink.js <opentsdbhost> <opentsdbport> <tagprefix>');
    process.exit(1);
}
var host = process.argv[2];
var port = process.argv[3];
var tagprefix = process.argv[4];


// Returns a list of "tagname=tagvalue" strings from the given metric name.
function parse_tags(metric_name) {
    var parts = metric_name.split(".");
    var tags = [];
    var current_tag_name = "";
    for (var i in parts)
    {
        var p = parts[i];
        if (p.indexOf(tagprefix) == 0)
        {
            current_tag_name = p.split(tagprefix)[1]
        }
        else if (current_tag_name != "")
        {
            tags.push(current_tag_name + "=" + p);
            current_tag_name = "";
        }
    }

    return tags;
}

// Strips out all tag information from the given metric name
function strip_tags(metric_name) {
    var parts = metric_name.split(".");
    var rslt_parts = [];
    while (parts.length > 0)
    {
        if (parts[0].indexOf(tagprefix) == 0)
        {
            parts.shift();
            parts.shift();
            continue;
        }
        rslt_parts.push(parts.shift());
    }

    return rslt_parts.join(".");
}

var data = '';

process.stdin.setEncoding('utf8');
process.stdin.on('readable', function () {
    var chunk = process.stdin.read();
    if(chunk !== null ) data += chunk
});
process.stdin.on('end', function(){
    var suffix = " source=statsd";
    if (data)
    {
        data = data.split('\n');
        var tString = '';
        for(var i in data)
        {
            var mParts = data[i].split('|');
            tString += ('put ' + strip_tags(mParts[0]) + ' ' + mParts[2] + ' ' + mParts[1] + ' ' + parse_tags(mParts[0]).join(' ') + suffix).replace(/\s+/g, ' ')+'\n';
        }
        try
        {
            var opentsdb = net.createConnection(port, host);
            opentsdb.on('connect', function () {
                this.write(tString);
                this.end();
                process.exit(0);
            });
        }
        catch (e)
        {
            console.log(e);
            process.exit(1);
        }
    }
});
