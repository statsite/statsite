
build:
	scons statsite

test_runner:
	scons test_runner

test: test_runner
	./test_runner

integ: build test
	py.test integ/

.PHONY: build test statsite_test

