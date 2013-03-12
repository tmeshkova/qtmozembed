%define qtmozembedversion 1.0.0

Name:       qtmozembed
Summary:    Qt MozEmbed
Version:    %{qtmozembedversion}
Release:    1
Group:      Applications/Internet
License:    Mozilla License
URL:        http://www.mozilla.com
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(QtCore) >= 4.6.0
BuildRequires:  pkgconfig(QtOpenGL)
BuildRequires:  pkgconfig(QtGui)
BuildRequires:  pkgconfig(QJson)
BuildRequires:  pkgconfig(libxul-embedding)

%description
Mozilla XUL runner

%package devel
Group: Development/Tools/Other
Requires: qtmozembed
Summary: Headers for qtmozembed

%description devel
Development files for qtmozembed.

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

%changelog
