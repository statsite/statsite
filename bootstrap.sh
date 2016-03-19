#! /bin/sh

set -x
aclocal -I ac_config
libtoolize --force --copy
#autoheader
automake --add-missing --copy
autoconf
