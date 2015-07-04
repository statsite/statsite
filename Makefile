RPMBUILDROOT=$(shell pwd)/rpm-build

build:
	scons statsite

clean:
	scons --clean test_runner
	scons --clean
	rm -rfv ./dist ./statsite ./rpm-build ./.sconsign.dblite ./.sconf_temp

test:
	scons test_runner
	./test_runner

integ: build test
	py.test integ/

sdist: clean
	mkdir -vp ./dist
	tar --exclude statsite.tar.gz -czvf ./dist/statsite.tar.gz .

rpms: sdist build
	mkdir -vp $(RPMBUILDROOT)
	cp -v dist/*.tar.gz $(RPMBUILDROOT)
	cp -v rpm/statsite.spec $(RPMBUILDROOT)
	rpmbuild --define "_topdir $(RPMBUILDROOT)" \
        --define "_builddir %{_topdir}" \
        --define "_rpmdir %{_topdir}" \
        --define "_srcrpmdir %{_topdir}" \
        --define "_specdir %{_topdir}" \
        --define '_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm' \
        --define "_sourcedir  %{_topdir}" \
        -ba $(RPMBUILDROOT)/statsite.spec

.PHONY: build test

