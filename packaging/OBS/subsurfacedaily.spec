#
# spec file for package subsurface
#
# Copyright (c) 2014 Dirk Hohndel
#

%define latestVersion 4.6.4.1031

%define gitVersion 1031


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
BuildRequires:	libssh2-devel
BuildRequires:	libcurl-devel
BuildRequires:	grantlee5-devel
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
BuildRequires:	qt5-qtconnectivity-devel
BuildRequires:	qt5-qtlocation-devel
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
BuildRequires:	libqt5-qtconnectivity-devel
BuildRequires:	libqt5-qtlocation-devel
BuildRequires:	libqt5-qtlocation-private-headers-devel
%endif
# Recommends Qt5 translations package
%if 0%{?suse_version}
Recommends:     libqt5-qttranslations
%endif
%if  0%{?fedora_version} >= 21
Recommends:     qt5-qttranslations
%endif
# Recommends debug info (and debug sources, for openSUSE) for daily build
%if %{name} == "subsurfacedaily"
Recommends:     %{name}-debuginfo
%if 0%{?suse_version}
Recommends:     %{name}-debugsource
%endif
%endif
BuildRoot:      %{_tmppath}/subsurface%{version}-build

%description
This is the official Subsurface test build, including our own custom libdivecomputer

%prep
%setup -q

%build
mkdir -p install-root
(cd libdivecomputer ; \
	autoreconf --install ; \
	./configure --prefix=$RPM_BUILD_DIR/install-root --disable-shared --disable-examples ; \
	make %{?_smp_mflags} ; \
	make install)
(cd libgit2; mkdir build; cd build; \
	cmake -DCMAKE_INSTALL_PREFIX=$RPM_BUILD_DIR/install-root -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_CLAR=OFF \
    	-DCMAKE_C_FLAGS:STRING="%optflags" \
		-DCMAKE_CXX_FLAGS:STRING="%optflags" \
        .. ; \
	make %{?_smp_mflags} ; \
	make install)
( cd googlemaps ; mkdir -p build ; cd build ; \
	qmake-qt5 "INCLUDEPATH=$INSTALL_ROOT/include" ../googlemaps.pro ; \
	make -j4 )
(mkdir subsurface-build ; cd subsurface-build ; \
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DLRELEASE=lrelease-qt5 \
		-DCMAKE_INSTALL_PREFIX=%{buildroot}/usr \
		-DLIBDIVECOMPUTER_INCLUDE_DIR=$RPM_BUILD_DIR/install-root/include \
		-DLIBGIT2_INCLUDE_DIR=$RPM_BUILD_DIR/install-root/include \
		-DLIBDIVECOMPUTER_LIBRARIES=$RPM_BUILD_DIR/install-root/lib/libdivecomputer.a \
		-DLIBGIT2_LIBRARIES=$RPM_BUILD_DIR/install-root/lib/libgit2.a \
		-DUSE_LIBGIT23_API=ON \
		-DCMAKE_C_FLAGS:STRING="%optflags" \
		-DCMAKE_CXX_FLAGS:STRING="%optflags" \
		-DNO_PRINTING=OFF \
		.. ; \
	make VERBOSE=1 %{?_smp_mflags} subsurface)

%install
mkdir -p %{buildroot}/%{_libdir}
(cd googlemaps/build ; make install_target INSTALL_ROOT=$RPM_BUILD_ROOT )
(cd subsurface-build ; make VERBOSE=1 install )
install subsurface.debug %{buildroot}%{_bindir}
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
%doc gpl-2.0.txt README.md ReleaseNotes/ReleaseNotes.txt
%{_bindir}/subsurface*
%{_libdir}/qt5/plugins/geoservices/libqtgeoservices_googlemaps.so
%{_datadir}/applications/subsurface.desktop
%{_datadir}/icons/hicolor/*/apps/subsurface-icon.*
%{_datadir}/subsurface/

%changelog

