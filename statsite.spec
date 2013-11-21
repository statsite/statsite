Name:		statsite
Version:	0.5.1
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
mkdir -vp $RPM_BUILD_ROOT/usr/bin
mkdir -vp $RPM_BUILD_ROOT/usr/libexec/%{name}
install -m 755 statsite $RPM_BUILD_ROOT/usr/bin
cp -a sinks $RPM_BUILD_ROOT/usr/libexec/%{name}

%clean
make clean

%files
%defattr(-,root,root,-)
%doc LICENSE
%doc CHANGELOG.md
%doc README.md
/usr/bin/statsite
/usr/libexec/statsite/sinks

%changelog
* Wed Nov 20 2013 Vito Laurenza <vitolaurenza@hotmail.com>
- Added 'sinks', which I overlooked initially.

* Fri Nov 15 2013 Vito Laurenza <vitolaurenza@hotmail.com>
- Initial release.
