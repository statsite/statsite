## Process this file with automake to produce Makefile.in

## run tests after everything else

SUBDIRS = lib src doc . tests

## FIXME: maybe we don't need this line

AM_MAKEINFOFLAGS = -I$(top_srcdir)/doc/example

## what to clean

CLEANFILES = *~\
	$(PACKAGE)-$(VERSION).tar.gz\
	ChangeLog.bak

## what additional things to distribute

EXTRA_DIST = SVNChangeLog ChangeLogOld check.pc.in $(m4data_DATA)

## install docs
docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = SVNChangeLog ChangeLog ChangeLogOld NEWS README COPYING.LESSER

## install check.m4 with AM_PATH_CHECK in it
m4datadir = $(datadir)/aclocal
m4data_DATA = check.m4

## install check.pc
pcdatadir = $(libdir)/pkgconfig
pcdata_DATA = check.pc

SVNChangeLog:
	if [ -e .svn ] ; then \
	svn log -v @abs_top_srcdir@ > $@ ;\
	else \
	echo "SVN Changelog not available" > $@ ;\
	fi

ACLOCAL_AMFLAGS = -I m4

doc/check.html:
	$(MAKE) -C doc check.html

# check we can do a clean build, including docs.
# perhaps we should check for out of date (svn st -u) and modified files.
prereleasecheck: doc/check.html
	-$(MAKE) distclean
	autoreconf -i && ./configure \
	    && ulimit -c 0 && \
	    $(MAKE) distcheck

RELEASE_VERSION ?= unset
SF_USERNAME ?= unset
releasevars:
	@if [ "unset" = "${RELEASE_VERSION}" ]; then \
	    echo "set RELEASE_VERSION to something like 0.9.6 to prepare a release."; \
	    false; \
	fi
	@if [ "unset" = "${SF_USERNAME}" ]; then \
	    echo "set SF_USERNAME to your sourceforge username"; \
	    false; \
	fi

.svnrelease:
	svn st -u | grep "Status against revision:" | awk '{ print $$4 }' > $@

preparerelease: releasevars .svnrelease prereleasecheck
	@echo checking we have can connect properly to do release tasks before tagging:
	sftp -b /dev/null ${SF_USERNAME}@web.sourceforge.net
	sftp -b /dev/null ${SF_USERNAME}@frs.sourceforge.net
	@echo tagging release from `cat .svnrelease`
	svn cp -m "Release check-${RELEASE_VERSION}" -r`cat .svnrelease` https://svn.sourceforge.net/svnroot/check/trunk https://svn.sourceforge.net/svnroot/check/tags/check-${RELEASE_VERSION}
	svn co https://svn.sourceforge.net/svnroot/check/tags/check-${RELEASE_VERSION} check-${RELEASE_VERSION}
	@echo "You need to:"
	@echo "Update TODO and make sure version numbers match appropriately."
	@echo "Update configure.ac with new version numbers."
	@echo "Then svn commit these changes."
	@echo "CTRL-D when finished or 'exit 1' to abort."
	@echo "aborting will leave the partially prepared release in" `pwd`
	cd check-${RELEASE_VERSION} && ${SHELL} && autoreconf -i && ./configure
	$(MAKE) -C check-${RELEASE_VERSION} dorelease
	-svn merge -r`cat .svnrelease`:`svn st -u check-${RELEASE_VERSION} | grep "Status against revision:" | awk '{ print $4 }'` check-${RELEASE_VERSION} .
	@echo "Please resolve any conflicts. Be sure to also restore the 'In Development:' line to NEWS."
	${SHELL}
	svn ci -m "Merge back release ${RELEASE_VERSION}"
	@echo "Cleaning up workdir."
	rm -rf check-${RELEASE_VERSION}


dorelease: releasevars doc/check.html prereleasecheck
	cp NEWS NEWS-${RELEASE_VERSION}.txt
	@echo "You need to:"
	@echo "cut out old release news from NEWS-${RELEASE_VERSION}.txt."
	@echo "CTRL-D to complete, or exit 1 to abort."
	${SHELL}
	@echo uploading...
	rsync -e ssh NEWS-${RELEASE_VERSION}.txt ${SF_USERNAME},check@frs.sourceforge.net:/home/frs/project/c/ch/check/check/${RELEASE_VERSION}/
	rsync -e ssh check-${RELEASE_VERSION}.tar.gz ${SF_USERNAME},check@frs.sourceforge.net:/home/frs/project/c/ch/check/check/${RELEASE_VERSION}/
	rsync -rlvC doc/. ${SF_USERNAME},check@web.sourceforge.net:/home/groups/c/ch/check/htdocs
	@echo
	@echo Uploaded...
	@echo "visit https://sourceforge.net/project/admin/explorer.php?group_id=28255"
	@echo "Mark the NEWS file in check/${RELEASE_VERSION} as being a release note."
	@echo "And the tar.gz file as being 'all platforms', using the NEWS file for release notes."
	@echo "visit https://sourceforge.net/projects/check/files/"
	@echo "Check it looks ok. It may take 15 minutes to show up."
	@echo Finally, mail check-devel ,announce and users at lists dot sourceforge dot net
	@echo an email (template is in HACKING) about this announcement.
	@echo NEWS-release can be found at `pwd`/NEWS-${RELEASE_VERSION}.txt
	@echo Note that with the tarballs, docs amd NEWS up, the release is done :P
	@echo "so you'll have to manually roll back if you abort."
	@echo "CTRL-D to complete, or exit 1 to abort."
	${SHELL}
	

.PHONY: releasevars prereleasecheck preparerelease dorelease .svnrelease
