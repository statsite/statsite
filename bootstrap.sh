#! /bin/sh

set -x

# Create directories in case they are missing
mkdir -p m4 ac_config


aclocal -I ac_config

# Execute libtoolize if we have it
command -v libtoolize >/dev/null 2>&1 && libtoolize --force --copy

# Execute glibtoolize as that's what OSX has, usually
command -v glibtoolize >/dev/null 2>&1 && glibtoolize --force --copy

#autoheader # We dont need this (yet?)

automake --add-missing --copy
autoconf
