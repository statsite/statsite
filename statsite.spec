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
install -m 755 statsite $RPM_BUILD_ROOT/usr/bin

%clean
make clean

%files
%defattr(-,root,root,-)
%doc LICENSE
%doc CHANGELOG.md
%doc README.md
/usr/bin/statsite

%changelog

