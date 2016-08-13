Statsite
========

Statsite is a metrics aggregation server. Statsite is based heavily
on Etsy's StatsD <https://github.com/etsy/statsd>, and is wire compatible.

Installation
------------

Statsite is built in C and uses scripting languages like Python as glue between
components and build steps. To setup your build environment, make sure you
have all required dependencies. At least the following is required when building
using the git sources:

- check (or as it's called, libcheck, see https://libcheck.github.io/check/)
- libtool
- automake
- autoconf
- gcc (or compatible C compiler)
- tex
- pytest
- scons (for legacy build and test scripts)

These dependencies have dependencies of their own, pytest requires python for
example, and installing pytest using a package management tool requires pip or
something similar.

Because we are using autotools, the system will warn you if something couldn't be
found, and if it's a critical error, it will stop instead of just disabling an
optional feature.

When using a source distribution, it's usually as simple as executing:

- ./configure
- make
- make install

When using the Git repo directly and not a source distribution tarball, additional
steps might be required to get autotools up. The steps are quite automated,
and contained within bootstrap.sh. It mostly consists of:

- aclocal
- libtoolize
- autoconf
- automake

after which a working platform-specific Makefile is ready for use. When working on
code you may need to re-run this, or maybe use the autoheader and autoreconf tools
to only update the parts you changed.


Dependencies
============

Most of the dependencies are included in standard Linux distributions,
but embedded toolchains and/or slightly less-POSIX systems may need special care
to get all dependencies installed.

Two internal dependencies are included with the project: ae and murmurhash.
For integration and unit tests, you need check (or, libcheck) and pytest. They
usually exist within your distribution's package repository or have standard
procedures for acquiring them.

Some distributions have specific packages you can use to make it easier for yourself:

Debian
------

On debian, check and pytest are in the standard repo's, as well as all build tools.
