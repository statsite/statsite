#! /bin/sh

set -x
mkdir m4 ac_config
aclocal -I ac_config
#libtoolize --force --copy
#autoheader
automake --add-missing --copy
autoconf
