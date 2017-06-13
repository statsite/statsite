
.PHONY: std condensed

std: ascii_parser_std.c
	mv $< ascii_parser.c

condensed: ascii_parser_condensed.c
	mv $< ascii_parser.c

ascii_parser_condensed.c: ascii_parser_condensed.rl
	ragel $^ -o $@

ascii_parser_std.c: ascii_parser_std.rl
	ragel $^ -o $@

ascii_parser.dot: ascii_parser_std.rl
	ragel $^ -V -p -o $@

ascii_parser.png: ascii_parser.dot
	dot -Tpng $^ -o $@
	eom $@

