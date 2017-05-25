
all: ascii_parser_condensed.c ascii_parser_std.c

ascii_parser_condensed.c: ascii_parser_condensed.rl
	ragel $^ -o $@

ascii_parser_std.c: ascii_parser_std.rl
	ragel $^ -o $@

ascii_parser.dot: ascii_parser_std.rl
	ragel $^ -V -p -o $@

ascii_parser.png: ascii_parser.dot
	dot -Tpng $^ -o $@
	eom $@

