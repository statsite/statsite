
#line 1 "ascii_parser_std.rl"
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

#line 47 "ascii_parser_std.rl"



void ascpp_emit(ascpp *parser, metric_type type) {
  parser->emit_cb(type,&parser->name, &parser->value, &parser->samplerate);
}


/** Data **/

#line 30 "ascii_parser_std.c"
static const char _ascii_protocol_parser_actions[] = {
	0, 1, 0, 1, 2, 1, 3, 1, 
	5, 1, 6, 1, 7, 1, 8, 1, 
	9, 1, 10, 1, 11, 2, 1, 0, 
	2, 4, 8
};

static const char _ascii_protocol_parser_key_offsets[] = {
	0, 0, 1, 2, 3, 4, 10, 12, 
	13, 14, 15, 16, 17, 18, 19, 20, 
	21, 22, 22
};

static const char _ascii_protocol_parser_trans_keys[] = {
	58, 58, 124, 124, 99, 103, 104, 107, 
	109, 115, 10, 124, 64, 10, 10, 10, 
	10, 118, 10, 115, 10, 10, 0
};

static const char _ascii_protocol_parser_single_lengths[] = {
	0, 1, 1, 1, 1, 6, 2, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 0, 0
};

static const char _ascii_protocol_parser_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0
};

static const char _ascii_protocol_parser_index_offsets[] = {
	0, 0, 2, 4, 6, 8, 15, 18, 
	20, 22, 24, 26, 28, 30, 32, 34, 
	36, 38, 39
};

static const char _ascii_protocol_parser_trans_targs[] = {
	0, 2, 3, 2, 0, 4, 5, 4, 
	6, 10, 11, 12, 14, 15, 0, 17, 
	7, 0, 8, 0, 0, 9, 17, 9, 
	17, 0, 17, 0, 13, 0, 17, 0, 
	11, 0, 17, 0, 18, 16, 0, 0, 
	0
};

static const char _ascii_protocol_parser_trans_actions[] = {
	17, 21, 3, 0, 17, 1, 5, 0, 
	0, 0, 0, 0, 0, 0, 17, 13, 
	0, 17, 0, 17, 17, 1, 24, 0, 
	9, 17, 11, 17, 0, 17, 7, 17, 
	0, 17, 15, 17, 19, 0, 0, 0, 
	0
};

static const char _ascii_protocol_parser_eof_actions[] = {
	0, 17, 17, 17, 17, 17, 17, 17, 
	17, 17, 17, 17, 17, 17, 17, 17, 
	0, 0, 0
};

static const int ascii_protocol_parser_start = 1;
static const int ascii_protocol_parser_first_final = 17;
static const int ascii_protocol_parser_error = 0;

static const int ascii_protocol_parser_en_main = 1;
static const int ascii_protocol_parser_en_line = 16;


#line 57 "ascii_parser_std.rl"


/** Public **/
ascpp ascpp_init(metric_cb cb) {
  int cs = 0;
  ascpp parser = {0};

  /* Init */
  
#line 110 "ascii_parser_std.c"
	{
	cs = ascii_protocol_parser_start;
	}

#line 66 "ascii_parser_std.rl"

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
  
#line 134 "ascii_parser_std.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _ascii_protocol_parser_trans_keys + _ascii_protocol_parser_key_offsets[cs];
	_trans = _ascii_protocol_parser_index_offsets[cs];

	_klen = _ascii_protocol_parser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _ascii_protocol_parser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = _ascii_protocol_parser_trans_targs[_trans];

	if ( _ascii_protocol_parser_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _ascii_protocol_parser_actions + _ascii_protocol_parser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 18 "ascii_parser_std.rl"
	{ MARK(p); }
	break;
	case 1:
#line 19 "ascii_parser_std.rl"
	{ CLEAR(name); CLEAR(value); CLEAR(samplerate);}
	break;
	case 2:
#line 20 "ascii_parser_std.rl"
	{ CAPTURE(name, p); }
	break;
	case 3:
#line 21 "ascii_parser_std.rl"
	{ CAPTURE(value, p); }
	break;
	case 4:
#line 22 "ascii_parser_std.rl"
	{ CAPTURE(samplerate, p); }
	break;
	case 5:
#line 23 "ascii_parser_std.rl"
	{ EMIT(KEY_VAL); }
	break;
	case 6:
#line 24 "ascii_parser_std.rl"
	{ EMIT(GAUGE); }
	break;
	case 7:
#line 25 "ascii_parser_std.rl"
	{ EMIT(TIMER); }
	break;
	case 8:
#line 26 "ascii_parser_std.rl"
	{ EMIT(COUNTER); }
	break;
	case 9:
#line 27 "ascii_parser_std.rl"
	{ EMIT(SET); }
	break;
	case 10:
#line 29 "ascii_parser_std.rl"
	{ ret=-1; p--; {cs = 16; goto _again;} }
	break;
	case 11:
#line 44 "ascii_parser_std.rl"
	{ {cs = 1; goto _again;} }
	break;
#line 255 "ascii_parser_std.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	const char *__acts = _ascii_protocol_parser_actions + _ascii_protocol_parser_eof_actions[cs];
	unsigned int __nacts = (unsigned int) *__acts++;
	while ( __nacts-- > 0 ) {
		switch ( *__acts++ ) {
	case 10:
#line 29 "ascii_parser_std.rl"
	{ ret=-1; p--; {cs = 16; goto _again;} }
	break;
#line 275 "ascii_parser_std.c"
		}
	}
	}

	_out: {}
	}

#line 84 "ascii_parser_std.rl"
}
