#
# spec file for package subsurface
#
# Copyright (c) 2014 Dirk Hohndel
#

%define latestVersion 4.4.1.363

Name:           subsurfacedaily
Version:	%latestVersion
Release:	0
License:	GPL v2
Summary:	Open source dive log
Url:		http://subsurface-divelog.org
Group:		Productivity/Other
Source:		subsurface-%latestVersion.orig.tar.xz
Conflicts:	subsurface
BuildRequires:	desktop-file-utils
BuildRequires:	fdupes
BuildRequires:	gcc-c++
BuildRequires:	make
BuildRequires:	asciidoc
BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	cmake
%if 0%{?suse_version}
# kde4-filesystem needed for some folders not owned (%{_datadir}/icons/hicolor and others)
BuildRequires:  kde4-filesystem
%endif
BuildRequires:	libzip-devel
BuildRequires:	libxml2-devel
BuildRequires:	libxslt-devel
%if  0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires:	netpbm-devel
BuildRequires:	openssl-devel
BuildRequires:	libsqlite3x-devel
BuildRequires:	libusbx-devel
BuildRequires:	qt5-qtbase-devel
BuildRequires:	qt5-qttools-devel
BuildRequires:	qt5-qtwebkit-devel
BuildRequires:	qt5-qtsvg-devel
BuildRequires:	qt5-qtscript-devel
BuildRequires:	qt5-qtdeclarative-devel
BuildRequires:	qt5-qtbase-mysql
BuildRequires:	qt5-qtbase-postgresql
BuildRequires:	qt5-qtbase-ibase
BuildRequires:	qt5-qtbase-odbc
BuildRequires:	qt5-qtbase-tds
%else
BuildRequires:	update-desktop-files
BuildRequires:	libopenssl-devel
BuildRequires:	sqlite3-devel
BuildRequires:	libusb-1_0-devel
BuildRequires:	libqt5-qtbase-devel
BuildRequires:	libqt5-qtsvg-devel
BuildRequires:	libqt5-linguist
BuildRequires:	libqt5-linguist-devel
BuildRequires:	libqt5-qttools-devel
BuildRequires:	libQt5WebKit5-devel
BuildRequires:	libQt5WebKitWidgets-devel
BuildRequires:	libqt5-qtscript-devel
BuildRequires:	libqt5-qtdeclarative-devel
%endif
# Recommends Qt5 translations package
%if 0%{?suse_version}
Recommends:     libqt5-qttranslations
%endif
%if  0%{?fedora_version} >= 21
Recommends:     qt5-qttranslations
%endif
BuildRoot:      %{_tmppath}/subsurface%{version}-build

%description
This is the official Subsurface build, including our own custom libdivecomputer and libssrfmarblewidget

%prep
%setup -q

%build
(cd libdivecomputer ; autoreconf --install ; ./configure --disable-shared ; make %{?_smp_mflags} )
(cd libgit2; mkdir build; cd build; cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF .. ; make )
(mkdir marble-build ; cd marble-build ; \
	cmake -DQTONLY=ON -DQT5BUILD=ON \
		-DBUILD_MARBLE_APPS=OFF -DBUILD_MARBLE_EXAMPLES=OFF \
		-DBUILD_MARBLE_TESTS=OFF -DBUILD_MARBLE_TOOLS=OFF \
		-DBUILD_TESTING=OFF -DWITH_DESIGNER_PLUGIN=OFF \
		-DBUILD_WITH_DBUS=OFF ../marble-source ; \
	make %{?_smp_mflags} ; \
	ln -s src/lib/marble lib ; \
	mkdir include ; cd include ; for i in `find ../../marble-source -name \*.h` ; do ln -s -f $i . ; done ; \
	ln -s -f . marble )
(mkdir subsurface-build ; cd subsurface-build ; \
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DLIBDCDEVEL=$(pwd)/../libdivecomputer -DLIBDCSTATIC=1 \
		-DLIBGIT2DEVEL=$(pwd)/../libgit2 -DLIBGIT2STATIC=1 \
		-DLIBMARBLEDEVEL=$(pwd)/../marble-build \
		-DLRELEASE=lrelease-qt5 \
		-DCMAKE_INSTALL_PREFIX=%{buildroot}/usr \
		$(pwd)/.. ; \
	make VERBOSE=1 %{?_smp_mflags} subsurface)

%install
mkdir -p %{buildroot}/%{_libdir}
(cd subsurface-build ; make VERBOSE=1 install )
%if 0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
desktop-file-install --dir=%{buildroot}/%{_datadir}/applications subsurface.desktop
%else
%suse_update_desktop_file -r subsurface Utility DesktopUtility
%endif
%fdupes %{buildroot}

%post
%desktop_database_post
/sbin/ldconfig

%postun
%desktop_database_post
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc gpl-2.0.txt README ReleaseNotes/ReleaseNotes.txt
%{_bindir}/subsurface
%{_datadir}/applications/subsurface.desktop
%{_datadir}/icons/hicolor/*/apps/subsurface-icon.*
%{_datadir}/subsurface/
/usr/lib*/libssrfmarblewidget.so*

%changelog

