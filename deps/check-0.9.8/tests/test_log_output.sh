#!/bin/sh

if [ "${srcdir}" = "." ]; then
    lsrc=""
else
    lsrc="${srcdir}/"
fi

expected="Running suite S1
${lsrc}ex_log_output.c:10:P:Core:test_pass:0: Passed
${lsrc}ex_log_output.c:16:F:Core:test_fail:0: Failure
${lsrc}ex_log_output.c:20:E:Core:test_exit:0: (after this point) Early exit with return value 1
Running suite S2
${lsrc}ex_log_output.c:28:P:Core:test_pass2:0: Passed
Results for all suites run:
50%: Checks: 4, Failures: 1, Errors: 1"


test_log_output ( ) {
    
    ./ex_log_output "${1}" > /dev/null
    actual=`cat test.log`
    if [ x"${expected}" != x"${actual}" ]; then
	echo "Problem with ex_log_output ${3}";
	echo "Expected:";
	echo "${expected}";
	echo "Got:";
	echo "${actual}";
	exit 1;
    fi
    
}

test_log_output "CK_SILENT";
test_log_output "CK_MINIMAL";
test_log_output "CK_NORMAL";
test_log_output "CK_VERBOSE";
exit 0
