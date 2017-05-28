#include <stdlib.h>
#include <string.h>

#include "config.h"

#define METRIC_TYPES 7

typedef struct {
  char *start;
  size_t len;
} token;

typedef void (*metric_cb)(metric_type type, 
    token *name, token *value, token *samplerate);

typedef struct {
  int cs;
  size_t mark;
  token name;
  token value;
  token samplerate;
  metric_cb emit_cb;
} ascpp;

ascpp ascpp_init(metric_cb cb);
void ascpp_exec(ascpp *parser, char *buffer, size_t len);

