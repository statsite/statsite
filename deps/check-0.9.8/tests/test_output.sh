#!/bin/sh

. ./test_vars

if [ "${srcdir}" = "." ]; then
    lsrc=""
else
    lsrc="${srcdir}/"
fi

t0="x"
t1="xRunning suite(s): Master
33%: Checks: 3, Failures: 1, Errors: 1"
t2="xRunning suite(s): Master
33%: Checks: 3, Failures: 1, Errors: 1
${lsrc}ex_output.c:17:F:Core:test_fail:0: Failure
${lsrc}ex_output.c:21:E:Core:test_exit:0: (after this point) Early exit with return value 1"
t3="xRunning suite(s): Master
33%: Checks: 3, Failures: 1, Errors: 1
${lsrc}ex_output.c:11:P:Core:test_pass:0: Passed
${lsrc}ex_output.c:17:F:Core:test_fail:0: Failure
${lsrc}ex_output.c:21:E:Core:test_exit:0: (after this point) Early exit with return value 1"
t4="xtest: Core:test_pass
success: Core:test_pass
test: Core:test_fail
failure: Core:test_fail [
${lsrc}ex_output.c:17: Failure
]
test: Core:test_exit
error: Core:test_exit [
${lsrc}ex_output.c:21: (after this point) Early exit with return value 1
]"

op0=`./ex_output CK_SILENT`
op1=`./ex_output CK_MINIMAL`
op2=`./ex_output CK_NORMAL`
op3=`./ex_output CK_VERBOSE`
if test 1 -eq $ENABLE_SUBUNIT; then
op4=`./ex_output CK_SUBUNIT`
fi


test_output ( ) {
    if [ "${1}" != "${2}" ]; then
	echo "Problem with ex_output ${3}";
	echo "Expected:";
	echo "${1}";
	echo "Got:";
	echo "${2}";
	exit 1;
    fi
    
}

test_output "$t0" x"$op0" "CK_SILENT";
test_output "$t1" x"$op1" "CK_MINIMAL";
test_output "$t2" x"$op2" "CK_NORMAL";
test_output "$t3" x"$op3" "CK_VERBOSE";
if test 1 -eq $ENABLE_SUBUNIT; then
test_output "$t4" x"$op4" "CK_SUBUNIT";
fi
exit 0
