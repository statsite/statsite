#include <syslog.h>
#include "sink.h"

/* Known sink constructors */
extern sink* init_stream_sink(const sink_config_stream*, const statsite_config*);

int init_sinks(sink** sinks, statsite_config* config) {
    for(sink_config* sc = config->sink_configs; sc != NULL; sc = sc->next) {
        if (sc->type == SINK_TYPE_STREAM) {
            sink* actual_sink = init_stream_sink((sink_config_stream*)sc, config);
            if (*sinks) {
                actual_sink->next = *sinks;
            }
            *sinks = actual_sink;
        } else {
            syslog(LOG_NOTICE, "Unknown sink type %d - we should have never gotten here as the config mis-matches the runtime configuration options.", sc->type);
            return 1;
        }
    }
    return 0;
}
