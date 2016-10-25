#!/usr/bin/env sh

OUTPUT_FILE=test.xml
CK_DEFAULT_TIMEOUT=4

. ./test_vars
. $(dirname $0)/test_output_strings

rm -f ${OUTPUT_FILE}
export CK_DEFAULT_TIMEOUT
./ex_output${EXEEXT} CK_MINIMAL XML NORMAL > /dev/null
actual_xml=`cat ${OUTPUT_FILE} | tr -d "\r" | grep -v \<duration\> | grep -v \<datetime\> | grep -v \<path\>`
if [ x"${expected_xml}" != x"${actual_xml}" ]; then
    echo "Problem with ex_xml_output${EXEEXT}";
    echo "Expected:";
    echo "${expected_xml}";
    echo "Got:";
    echo "${actual_xml}";
    exit 1;
fi

actual_duration_count=`grep -c \<duration\> ${OUTPUT_FILE}`
if [ x"${expected_duration_count}" != x"${actual_duration_count}" ]; then
    echo "Wrong number of <duration> elements in ${OUTPUT_FILE}, ${expected_duration_count} vs ${actual_duration_count}";
    exit 1;
fi

for duration in `grep "\<duration\>" ${OUTPUT_FILE} | cut -d ">" -f 2 | cut -d "<" -f 1`; do
int_duration=`echo $duration | cut -d "." -f 1`
if [ "${int_duration}" -ne "-1" ] && [ "${int_duration}" -gt "${CK_DEFAULT_TIMEOUT}" ]; then
    echo "Problem with duration ${duration}; is not valid. Should be -1 or in [0, ${CK_DEFAULT_TIMEOUT}]"
    exit 1
fi
done


exit 0
