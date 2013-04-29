Name:       qtmozembed
Summary:    Qt embeddings for Gecko
Version:    1.2.3
Release:    1
Group:      Applications/Internet
License:    Mozilla License
URL:        https://github.com/tmeshkova/qtmozembed.git
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore) >= 4.6.0
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(QJson)
BuildRequires:  pkgconfig(libxul-embedding)
BuildRequires:  pkgconfig(nspr)
BuildRequires:  pkgconfig(QtTest)
BuildRequires:  qtest-qml-devel

%description
Qt embeddings for Gecko browser engine

%package devel
Group:      Applications/Internet
Requires:   %{name} = %{version}-%{release}
Summary:    Headers for qtmozembed

%description devel
Development files for qtmozembed.

%package tests
Summary:    Unit tests for QtMozEmbed tests
Group:      Applications/Internet
Requires:   %{name} = %{version}-%{release}
Requires:   embedlite-components >= 1.0.10

%description tests
This package contains QML unit tests for QtMozEmbed library

%prep
%setup -q -n %{name}-%{version}

%build
qmake
%{__make} %{?jobs:MOZ_MAKE_FLAGS="-j%jobs"}

%install
%{__make} install INSTALL_ROOT=%{buildroot}
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_libdir}/pkgconfig
%{_includedir}/*

%files tests
%defattr(-,root,root,-)
# >> files tests
/opt/tests/qtmozembed/*
%{_libdir}/qt4/bin/*
# << files tests
