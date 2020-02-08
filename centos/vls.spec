Name: vls
Version: 0.2
Release: 1%{?dist}
Summary: vls is a software
Group: Applications/System
License:GPL
URL: http://github.com/keminar/vls
Source0: %{name}-%{version}.tar.gz

BuildRequires: gcc,make

%description

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%files
/usr/bin/vls

%doc

%changelog
