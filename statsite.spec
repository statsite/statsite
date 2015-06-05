%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

Name:		statsite
Version:	0.7.1
Release:	2%{?dist}
Summary:	A C implementation of statsd.
Group:		Applications
License:	See the LICENSE file.
URL:		https://github.com/twitter-forks/statsite
Source0:	statsite.tar.gz
Requires:       %{!?el5:libcurl} %{?el5:curl} jansson
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires:	scons check-devel %{?el7:systemd} %{?fedora:systemd} %{?el5:curl-devel} %{!?el5:libcurl-devel} jansson-devel
AutoReqProv:	No
Requires(pre):  /usr/sbin/useradd, /usr/bin/getent

%description

Statsite is a metrics aggregation server. Statsite is based heavily on Etsy\'s StatsD
https://github.com/etsy/statsd, and is wire compatible.

%prep
%setup -c %{name}-%{version}

%build
make %{?_smp_mflags}

%install
mkdir -vp $RPM_BUILD_ROOT/usr/sbin
mkdir -vp $RPM_BUILD_ROOT/etc/init.d
mkdir -vp $RPM_BUILD_ROOT/etc/%{name}
mkdir -vp $RPM_BUILD_ROOT/usr/libexec/%{name}
mkdir -vp $RPM_BUILD_ROOT/var/run/statsite

%if 0%{?fedora}%{?el7}
mkdir -vp $RPM_BUILD_ROOT/%{_unitdir}
install -m 644 rpm/statsite.service $RPM_BUILD_ROOT/%{_unitdir}
%else
install -m 755 rpm/statsite.initscript $RPM_BUILD_ROOT/etc/init.d/statsite
%endif

install -m 755 statsite $RPM_BUILD_ROOT/usr/sbin
install -m 644 rpm/statsite.conf.example $RPM_BUILD_ROOT/etc/%{name}/statsite.conf
cp -a sinks $RPM_BUILD_ROOT/usr/libexec/%{name}

%clean

make clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%pre
/usr/bin/getent group statsite >/dev/null 2>&1 || \
  /usr/sbin/groupadd -r statsite >/dev/null 2>&1 || :;
useradd_option=%{?el5:-n}%{!?el5:-N}

/usr/bin/getent passwd statsite >/dev/null 2>&1 || \
    /usr/sbin/useradd  -r "${useradd_option}" -M -g statsite \
      -c "statsite Service Account" \
      -s /sbin/nologin statsite >/dev/null 2>&1 || :;

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
%config(noreplace) /etc/%{name}/statsite.conf
%attr(755, root, root) /usr/sbin/statsite
%if 0%{?fedora}%{?el7}
%attr(644, root, root) %{_unitdir}/statsite.service
%else
%attr(755, root, root) /etc/init.d/statsite
%endif
%dir /usr/libexec/statsite
%dir /usr/libexec/statsite/sinks
%dir /var/run/statsite
%attr(755, statsite, statsite) /var/run/statsite
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
* Fri Jun 05 2015 Yann Ramin - 0.7.1-2
- Fix user creation in statsite

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
