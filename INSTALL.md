Statsite
========

Statsite is a metrics aggregation server. Statsite is based heavily
on Etsy's StatsD <https://github.com/etsy/statsd>, and is wire compatible.

Installation
------------

Statsite is built in C and uses scripting languages like Python as glue between
components and build steps. To setup your build environment, make sure you
have all required dependencies. At least the following is required when building
using the git sources, and running tests:

- check (or as it's called, libcheck, see https://libcheck.github.io/check/)
- libtool
- automake
- autoconf
- gcc (or compatible C compiler)
- tex (if you're recompiling check)
- pytest
- requests (python-requests)
- scons (for legacy build and test scripts)

These dependencies have dependencies of their own, pytest requires python for
example, and installing pytest using a package management tool requires pip or
something similar.

Because we are using autotools, the system will warn you if something couldn't be
found, and if it's a critical error, it will stop.

Building
--------

When using a non-git source tarball distribution, it's usually as simple as executing:

~~~~
./configure
make
make install
~~~~

This will get you a plain local installation of statsite, no tests,
no integration tests and no special packages.

When using the git repo directly and not a source distribution tarball, additional
steps might be required to get autotools up. The steps are quite automated,
and contained within autogen.sh. It mostly consists of the bare mininum:

- aclocal
- libtoolize or glibtoolize
- automake
- autoconf

after which a working platform-specific Makefile is ready for use. When working on
code you may need to re-run this, or maybe use the autoheader and autoreconf tools
to only update the parts you changed. Running autogen.sh also works, but might take
a few seconds longer to run.


Dependencies
============

Most of the dependencies are included in standard Linux distributions,
but embedded toolchains and/or slightly less-POSIX systems may need special care
to get all dependencies installed.

Internal dependencies are included with the project: inih, ae and murmurhash.
For integration and unit tests, you need check (or, libcheck) and pytest. They
usually exist within your distribution's package repository or have standard
procedures for acquiring them.

The `check` or `libcheck` as it is called can be built from source if you wish.
We included check-0.10.0 in the git tree as this is what was used since the first release.
To build this, you will need to get the dependencies setup as listed in deps/check-0.10.0/INSTALL.
Currently, we the requirements are:

- automake-1.9.6 (1.11.3 on OS X if you are using /usr/bin/ar)
- autoconf-2.59
- libtool-1.5.22
- pkg-config-0.20
- texinfo-4.7 (for documentation)
- tetex-bin (or any texinfo-compatible TeX installation, for documentation)
- POSIX sed

Check should be easy to build, a standard setup will do:

~~~~
# make sure you are in the check directory
cd check-0.10.0
./configure
make
make install
~~~~

This is how the packaged `check` gets built, and how source distributions are built.
If you use a git version, you may need to setup its autotools parts,
check has it's own install docs on that.

For python test dependencies, you can use pip/easy_install, or if you really want
to do it manually, see below links for the python packages required.

- pytest (https://github.com/pytest-dev/pytest)
- requests (http://github.com/requests/requests)

Distro-specifics
================

Some distributions have specific packages you can use to make it easier for yourself,
the following have been tested:

Debian
------

On Debian, check and pytest are in the standard repos, as well as all build tools.
Install the packages according to the following snippet:

~~~~
sudo apt-get update
sudo apt-get -y install build-essential libtool autoconf automake scons python-setuptools lsof git texlive check
sudo easy_install pip
sudo pip install pytest requests
~~~~

You can then run autogen.sh to kick off autotools:
~~~~
# be sure you are in the statsite directory
cd statsite
./autogen.sh
~~~~

afterwards you can configure the build and build it, with the option to directly install afterwards:
~~~~
# be sure you are in the statsite directory if you weren't already
cd statsite
./configure
make
sudo make install
~~~~

The statsite binary after installation is linked like this on Debian 8:
~~~~
linux-vdso.so.1 (0x00007ffd43ba1000)
libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fa2e1ccf000)
librt.so.1 => /lib/x86_64-linux-gnu/librt.so.1 (0x00007fa2e1ac7000)
libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007fa2e18aa000)
libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fa2e14ff000)
/lib64/ld-linux-x86-64.so.2 (0x00007fa2e1fd0000)
~~~~

If you want to run unit tests and integration tests, you will need a few extra packages:

~~~~
sudo apt-get update
sudo apt-get -y install build-essential libtool autoconf automake scons python-setuptools lsof git texlive check
sudo easy_install pip
sudo pip install pytest requests
~~~~

with those packages installed you can run the tests:
~~~~
# be sure you are in the statsite directory if you weren't already
cd statsite
./autogen.sh
./configure
make test
make integ
~~~~




CentOS
------

Install packages:

yum groupinstall 'Development Tools'
yum install check
