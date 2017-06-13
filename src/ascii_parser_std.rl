#include "ascii_parser.h"

#define MARK(FPC) parser->mark = FPC - buffer
#define CAPTURE(LABEL, FPC) do { \
  parser->LABEL.start = buffer + parser->mark; \
  parser->LABEL.len = FPC - buffer - parser->mark; \
} while (0)
#define CLEAR(LABEL) do { \
  parser->LABEL.start = NULL; \
  parser->LABEL.len = 0; \
} while (0)
#define EMIT(T) ( ascpp_emit(parser, T) )

/** Machine **/
%%{
  machine ascii_protocol_parser;

  action Mark           { MARK(fpc); }
  action Reset          { CLEAR(name); CLEAR(value); CLEAR(samplerate);}
  action Name           { CAPTURE(name, fpc); }
  action Value          { CAPTURE(value, fpc); }
  action SampleRate     { CAPTURE(samplerate, fpc); }
  action KeyValue       { EMIT(KEY_VAL); }
  action Gauge          { EMIT(GAUGE); }
  action Timer          { EMIT(TIMER); }
  action Counter        { EMIT(COUNTER); }
  action Set            { EMIT(SET); }

  action stat_err        { ret=-1; fhold; fgoto line; }

  valstr       = (any - '|')+                       >Mark %Value;
  name         = (any - ':')+                       >Mark %Name;
  samplerate   = (any - '\n')+                      >Mark %SampleRate;

  keyvalue     = valstr '|kv'                       %KeyValue;
  gauge        = valstr '|g'                        %Gauge;
  timer        = valstr '|' ('h' | 'ms')            %Timer;
  counter      = valstr ('|c' ('|@' samplerate)?)   %Counter;
  set          = valstr '|s'                        %Set;

  stat         = (name (':' (keyvalue | gauge | timer | counter | set))) >Reset $err(stat_err); 

  main :=  stat '\n';
  line := [^\n]* '\n' @{ fgoto main; };


}%%


void ascpp_emit(ascpp *parser, metric_type type) {
  parser->emit_cb(type,&parser->name, &parser->value, &parser->samplerate);
}


/** Data **/
%% write data;


/** Public **/
ascpp ascpp_init(metric_cb cb) {
  int cs = 0;
  ascpp parser = {0};

  /* Init */
  %% write init;

  parser.cs = cs;
  parser.mark = 0;
  parser.emit_cb = cb;

	return parser;
}

void ascpp_exec(ascpp *parser, char *buffer, size_t len) {
  const char *p, *pe, *eof;
  int cs = parser->cs, ret=0;

  p = buffer;
  pe = buffer+len;
  eof = buffer+len;

  /* Exec */
  %% write exec;
}
