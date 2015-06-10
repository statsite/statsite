#include <syslog.h>
#include "sink.h"

/* Known sink constructors */
extern sink* init_stream_sink(const sink_config_stream*, const statsite_config*);
extern sink* init_http_sink(const sink_config_http*, const statsite_config*);

int init_sinks(sink** sinks, statsite_config* config) {
    for(sink_config* sc = config->sink_configs; sc != NULL; sc = sc->next) {
        switch(sc->type) {
        case SINK_TYPE_STREAM:
        {
            sink* actual_sink = init_stream_sink((sink_config_stream*)sc, config);
            actual_sink->next = *sinks;
            *sinks = actual_sink;
            break;
        }
        case SINK_TYPE_HTTP:
        {
            sink* actual_sink = init_http_sink((sink_config_http*)sc, config);
            actual_sink->next = *sinks;
            *sinks = actual_sink;
            break;
        }
        default:
            syslog(LOG_NOTICE, "Unknown sink type %d - we should have never gotten here as the config mis-matches the runtime configuration options.", sc->type);
            return 1;
        }
    }
    return 0;
}
