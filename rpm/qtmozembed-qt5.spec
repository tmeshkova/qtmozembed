%global min_xulrunner_version 38.0.5.3

Name:       qtmozembed-qt5
Summary:    Qt embeddings for Gecko
Version:    1.13.2
Release:    1
Group:      Applications/Internet
License:    Mozilla License
URL:        https://github.com/tmeshkova/qtmozembed.git
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5QuickTest)
BuildRequires:  pkgconfig(nspr)
BuildRequires:  xulrunner-qt5-devel >= %{min_xulrunner_version}
BuildRequires:  qt5-default
BuildRequires:  qt5-qttools
Requires:       xulrunner-qt5 >= %{min_xulrunner_version}

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
Requires:   embedlite-components-qt5 >= 1.0.0
Requires:   qt5-qtdeclarative-import-qttest
Requires:   mce-tools

%description tests
This package contains QML unit tests for QtMozEmbed library

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 VERSION=%{version}
%{__make} %{?jobs:MOZ_MAKE_FLAGS="-j%jobs"}

%install
%qmake5_install
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_libdir}/qt5/qml/Qt5Mozilla/*

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.so
%{_libdir}/pkgconfig
%{_includedir}/*

%files tests
%defattr(-,root,root,-)
# >> files tests
/opt/tests/qtmozembed/*
%{_libdir}/qt5/bin/*
# << files tests
