%global __os_install_post %(echo '%{__os_install_post}' | sed -e 's!/usr/lib[^[:space:]]*/brp-python-bytecompile[[:space:]].*$!!g')

Name:		statsite
Version:	0.7.0
Release:	1%{?dist}
Summary:	A C implementation of statsd.
Group:		Applications
License:	See the LICENSE file.
URL:		https://github.com/armon/statsite
Source0:	statsite.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
BuildRequires:	scons
AutoReqProv:	No

%description

Statsite is a metrics aggregation server. Statsite is based heavily on Etsy's StatsD
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
install -m 755 statsite $RPM_BUILD_ROOT/usr/sbin
install -m 755 rpm/statsite.initscript $RPM_BUILD_ROOT/etc/init.d/statsite
install -m 644 rpm/statsite.conf.example $RPM_BUILD_ROOT/etc/%{name}/statsite.conf
cp -a sinks $RPM_BUILD_ROOT/usr/libexec/%{name}

%clean
make clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%post
/sbin/chkconfig --add %{name}
/sbin/chkconfig %{name} off

%preun
if [ "$1" = 0 ] ; then
    %{monit_bin} stop %{appname}
    /sbin/service %{appname} stop > /dev/null 2>&1
    /sbin/chkconfig --del %{appname}
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
%attr(755, root, root) /etc/init.d/statsite
%dir /usr/libexec/statsite
%dir /usr/libexec/statsite/sinks
%attr(755, root, root) /usr/libexec/statsite/sinks/__init__.py
%attr(755, root, root) /usr/libexec/statsite/sinks/binary_sink.py
%attr(755, root, root) /usr/libexec/statsite/sinks/librato.py
%attr(755, root, root) /usr/libexec/statsite/sinks/statsite_json_sink.rb
%attr(755, root, root) /usr/libexec/statsite/sinks/gmetric.py
%attr(755, root, root) /usr/libexec/statsite/sinks/influxdb.py
%attr(755, root, root) /usr/libexec/statsite/sinks/graphite.py

%changelog
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
