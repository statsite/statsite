## Process this file with automake to produce Makefile.in

## run tests after everything else

SUBDIRS = lib src doc . checkmk tests

## FIXME: maybe we don't need this line

AM_MAKEINFOFLAGS = -I$(top_srcdir)/doc/example

## what to clean

CLEANFILES = *~\
	$(PACKAGE)-$(VERSION).tar.gz\
	ChangeLog.bak

## what additional things to distribute

include_HEADERS = check_stdint.h

EXTRA_DIST = SVNChangeLog ChangeLogOld check.pc.in $(m4data_DATA) xml/check_unittest.xslt \
	CMakeLists.txt src/CMakeLists.txt tests/CMakeLists.txt lib/CMakeLists.txt \
	cmake

## install docs
docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = SVNChangeLog ChangeLog ChangeLogOld NEWS README COPYING.LESSER

## install check.m4 with AM_PATH_CHECK in it
m4datadir = $(datadir)/aclocal
m4data_DATA = check.m4

## install check.pc
pcdatadir = $(libdir)/pkgconfig
pcdata_DATA = check.pc

DISTCLEANFILES = check_stdint.h

SVNChangeLog:
	if [ -e .svn ] ; then \
	svn log -v @abs_top_srcdir@ > $@ ;\
	else \
	echo "SVN Changelog not available" > $@ ;\
	fi

ACLOCAL_AMFLAGS = -I m4

doc/check_html:
	$(MAKE) -C doc check_html

doc/doxygen:
	$(MAKE) -C doc doxygen

# check we can do a clean build, including docs.
# perhaps we should check for out of date (svn st -u) and modified files.
prereleasecheck: doc/check_html doc/doxygen
	-$(MAKE) distclean
	autoreconf -i && ./configure \
	    && ulimit -c 0 && \
	    $(MAKE) distcheck

RELEASE_VERSION ?= unset
SF_USERNAME ?= unset

releaseversioncheck:
	@if [ "unset" = "${RELEASE_VERSION}" ]; then \
	    echo "set RELEASE_VERSION to something like 0.9.6 to prepare a release."; \
	    false; \
	fi

fsusernamecheck:
	@if [ "unset" = "${SF_USERNAME}" ]; then \
	    echo "set SF_USERNAME to your sourceforge username"; \
	    false; \
	fi

releasevars: releaseversioncheck fsusernamecheck

.svnrelease:
	svn st -u | grep "Status against revision:" | awk '{ print $$4 }' > $@

preparerelease: releasevars .svnrelease prereleasecheck
	@echo tagging release from `cat .svnrelease`
	svn cp -m "Tagging for check-${RELEASE_VERSION}" -r`cat .svnrelease` https://svn.code.sf.net/p/check/code/trunk https://svn.code.sf.net/p/check/code/tags/check-${RELEASE_VERSION}
	svn co https://svn.code.sf.net/p/check/code/tags/check-${RELEASE_VERSION} check-${RELEASE_VERSION}
	@echo "Any last minute changes? This is your final chance."
	@echo "CTRL-D when finished or 'exit 1' to abort."
	@echo "aborting will leave the partially prepared release in" `pwd`
	@echo "If doing the rest manually, don't forget to"
	@echo "'make doc/check_html' before rsyncing the release directory."
	cd check-${RELEASE_VERSION} && ${SHELL} && svn ci -m "Release check-${RELEASE_VERSION}" && autoreconf -i && ./configure
	$(MAKE) -C check-${RELEASE_VERSION} dorelease
	-svn merge -r`cat .svnrelease`:`svn st -u check-${RELEASE_VERSION} | grep "Status against revision:" | awk '{ print $$4 }'` check-${RELEASE_VERSION} .
	@echo "Please resolve any conflicts. Be sure to also restore the 'In Development:' line to NEWS."
	${SHELL}
	svn ci -m "Merge back release ${RELEASE_VERSION}"
	@echo "Cleaning up workdir."
	rm -rf check-${RELEASE_VERSION}


dorelease: releasevars doc/check_html doc/doxygen prereleasecheck
	cp NEWS NEWS-${RELEASE_VERSION}.txt
	@echo "You need to:"
	@echo "cut out old release news from NEWS-${RELEASE_VERSION}.txt."
	@echo "CTRL-D to complete, or exit 1 to abort."
	${SHELL}
	@echo "You need to:"
	@echo "Update the release information in index.html"
	@echo "The current date and release number need to be updated."
	@echo "Look for the string: <!-- Update this section during a release -->"
	@echo "CTRL-D to complete, or exit 1 to abort."
	${SHELL}
	@echo uploading...
	rsync -e ssh NEWS-${RELEASE_VERSION}.txt ${SF_USERNAME},check@frs.sourceforge.net:/home/frs/project/c/ch/check/check/${RELEASE_VERSION}/
	rsync -e ssh check-${RELEASE_VERSION}.tar.gz ${SF_USERNAME},check@frs.sourceforge.net:/home/frs/project/c/ch/check/check/${RELEASE_VERSION}/
	rsync -e ssh -rlvC . ${SF_USERNAME},check@web.sourceforge.net:htdocs
	@echo
	@echo "Uploaded..."
	@echo "visit https://sourceforge.net/projects/check/files/"
	@echo "Mark the NEWS file in check/${RELEASE_VERSION} as being a"
	@echo "README by clicking the 'i' by the file and change the name to"
	@echo "NEWS-README-${RELEASE_VERSION}."
	@echo "Click 'i' by the tar.gz file and at 'Default download for:'"
	@echo "click 'select all', then save."
	@echo "visit https://sourceforge.net/projects/check/files/"
	@echo "Check it looks ok. It may take 15 minutes to show up."
	@echo "Finally, mail"
	@echo "check-devel ,announce and users at lists dot sourceforge dot net"
	@echo "an announcement (template is in HACKING) about this release."
	@echo "NEWS-release can be found at `pwd`/NEWS-${RELEASE_VERSION}.txt"
	@echo "Note that with the tarballs, docs amd NEWS up, the release"
	@echo "is done :P so you'll have to manually roll back if you abort."
	@echo "CTRL-D to complete, or exit 1 to abort."
	${SHELL}

updateweb: fsusernamecheck
	@echo uploading...
	rsync -e ssh -rlvC index.html web doc ${SF_USERNAME},check@web.sourceforge.net:htdocs

.PHONY: releasevars prereleasecheck preparerelease dorelease .svnrelease
