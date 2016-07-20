%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

Name:		statsite
Version:	0.8.0
Release:	1%{?dist}
Summary:	A C implementation of statsd.
Group:		Applications
License:	See the LICENSE file.
URL:		https://github.com/armon/statsite
Source0:	%{name}-%{version}.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires:	scons check-devel %{?el7:systemd} %{?fedora:systemd}
AutoReqProv:	No
Requires(pre):  shadow-utils

%description

Statsite is a metrics aggregation server. Statsite is based heavily on Etsy\'s StatsD
https://github.com/etsy/statsd, and is wire compatible.

%pre
getent group %{name} >/dev/null || groupadd -r %{name}
getent passwd  %{name}  >/dev/null || \
    useradd -r -g  %{name} -d /var/lib/%{name} -s /sbin/nologin \
    -c "Statsite user" %{name} 
exit 0

%prep
%setup

%build
make %{?_smp_mflags}

%install
mkdir -vp $RPM_BUILD_ROOT/usr/sbin
mkdir -vp $RPM_BUILD_ROOT/etc/init.d
mkdir -vp $RPM_BUILD_ROOT/etc/%{name}
mkdir -vp $RPM_BUILD_ROOT/etc/tmpfiles.d
mkdir -vp $RPM_BUILD_ROOT/usr/libexec/%{name}
mkdir -vp $RPM_BUILD_ROOT/var/run/%{name}
mkdir -vp $RPM_BUILD_ROOT/var/lib/%{name}

%if 0%{?fedora}%{?el7}
mkdir -vp $RPM_BUILD_ROOT/%{_unitdir}
install -m 644 rpm/statsite.service $RPM_BUILD_ROOT/%{_unitdir}
install -m 644 rpm/statsite.tmpfiles.conf $RPM_BUILD_ROOT/etc/tmpfiles.d/statsite.conf
%else
install -m 755 rpm/statsite.initscript $RPM_BUILD_ROOT/etc/init.d/statsite
%endif

install -m 755 src/statsite $RPM_BUILD_ROOT/usr/sbin
install -m 644 rpm/statsite.conf.example $RPM_BUILD_ROOT/etc/%{name}/statsite.conf
cp -a sinks $RPM_BUILD_ROOT/usr/libexec/%{name}

%clean
make clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%post
if [ "$1" = 1 ] ; then
%if 0%{?fedora}%{?el7}
	systemctl daemon-reload
%else
	/sbin/chkconfig --add %{name}
	/sbin/chkconfig %{name} off
%endif

fi
exit 0

%postun
if [ "$1" = 1 ] ; then
%if 0%{?fedora}%{?el7}
	systemctl restart statsite.service
%else
	/sbin/service %{name} restart

%endif
fi
exit 0

%preun
if [ "$1" = 0 ] ; then
	%if 0%{?monit_bin}
	%{monit_bin} stop %{name}
	%endif

%if 0%{?fedora}%{?el7}
	systemctl stop statsite.service
%else
	/sbin/service %{name} stop > /dev/null 2>&1
	/sbin/chkconfig --del %{name}
%endif
fi
exit 0

%files
%defattr(-,root,root,-)
%doc LICENSE
%doc CHANGELOG.md
%doc README.md
%doc rpm/statsite.conf.example
%config /etc/%{name}/statsite.conf
%attr(755, root, root) /usr/sbin/statsite
%if 0%{?fedora}%{?el7}
%attr(644, root, root) %{_unitdir}/statsite.service
%dir /etc/tmpfiles.d
%attr(644, root, root) /etc/tmpfiles.d/statsite.conf
%else
%attr(755, root, root) /etc/init.d/statsite
%endif
%dir /usr/libexec/statsite
%dir /usr/libexec/statsite/sinks
%attr(755, statsite, statsite) /var/run/statsite
%attr(755, statsite, statsite) /var/lib/statsite
%attr(755, root, root) /usr/libexec/statsite/sinks/__init__.py
%attr(755, root, root) /usr/libexec/statsite/sinks/binary_sink.py
%attr(755, root, root) /usr/libexec/statsite/sinks/librato.py
%attr(755, root, root) /usr/libexec/statsite/sinks/statsite_json_sink.rb
%attr(755, root, root) /usr/libexec/statsite/sinks/gmetric.py
%attr(755, root, root) /usr/libexec/statsite/sinks/influxdb.py
%attr(755, root, root) /usr/libexec/statsite/sinks/graphite.py
%attr(755, root, root) /usr/libexec/statsite/sinks/cloudwatch.sh
%attr(755, root, root) /usr/libexec/statsite/sinks/opentsdb.js

%changelog
* Tue May 12 2015 Yann Ramin <yann@twitter.com> - 0.7.1-1
- Add a statsite user and group
- Add systemd support

* Fri Jul 18 2014 Gary Richardson <gary.richardson@gmail.com>
- added missing __init__.py to spec file
- fixed makefile for building RPMS

* Tue May 20 2014 Marcelo Teixeira Monteiro <marcelotmonteiro@gmail.com>
- Added initscript and config file
- small improvements

* Wed Nov 20 2013 Vito Laurenza <vitolaurenza@hotmail.com>
- Added 'sinks', which I overlooked initially.

* Fri Nov 15 2013 Vito Laurenza <vitolaurenza@hotmail.com>
- Initial release.
