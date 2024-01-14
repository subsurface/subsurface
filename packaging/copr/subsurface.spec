#
# Fedora COPR spec file for package subsurface
#
# Copyright (c) 2014-2022 Dirk Hohndel
#

%define latestVersion 0.0.0.0

Name:           subsurface
Version:	%latestVersion
Release:        1%{?dist}

Summary:        SUMMARY

License:        GPL v2
Url:            http://subsurface-divelog.org

Source:		subsurface-%latestVersion.orig.tar.xz

Group:          Productivity/Other

BuildRequires:  desktop-file-utils
BuildRequires:  fdupes
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  asciidoc
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  cmake
BuildRequires:  libzip-devel
BuildRequires:  libxml2-devel
BuildRequires:  libxslt-devel
BuildRequires:  libssh2-devel
BuildRequires:  libcurl-devel
BuildRequires:  libgit2-devel
BuildRequires:  libmtp-devel
BuildRequires:  netpbm-devel
BuildRequires:  openssl-devel
BuildRequires:  libsqlite3x-devel
BuildRequires:  libusbx-devel
BuildRequires:  bluez-libs-devel
BuildRequires:  qt5-qtbase-devel
BuildRequires:  qt5-qttools-devel
BuildRequires:  qt5-qtwebkit-devel
BuildRequires:  qt5-qtsvg-devel
BuildRequires:  qt5-qtscript-devel
BuildRequires:  qt5-qtdeclarative-devel
BuildRequires:  qt5-qtbase-mysql
BuildRequires:  qt5-qtbase-postgresql
BuildRequires:  qt5-qtbase-ibase
BuildRequires:  qt5-qtbase-odbc
BuildRequires:  qt5-qtbase-tds
BuildRequires:  qt5-qtconnectivity-devel
BuildRequires:  qt5-qtlocation-devel
BuildRequires:  libappstream-glib

Recommends:     qt5-qttranslations

%description
DESCRIPTION


%prep
%setup -q



%build
# we need to temporarily install the output of our two included dependency in order to use those
# when building Subsurface - yes, this is ugly. But since we have private versions of these two
# libraries, this is the most reasonable approach...
mkdir -p install-root
(cd libdivecomputer ; \
        autoreconf --install ; \
        CFLAGS="-fPIC -g -O2" ./configure --prefix=%{_builddir}/install-root --bindir=%{_builddir}/install-root/bin --libdir=%{_builddir}/install-root/lib --includedir=%{_builddir}/install-root/include --disable-examples ; \
        make %{?_smp_mflags} ; \
        make install)
( cd googlemaps ; mkdir -p build ; cd build ; \
        qmake-qt5 ../googlemaps.pro ; \
        # on Fedora 36 and newer we get the package_notes that break the build - let's rip them out
        sed -i 's/-Wl[^ ]*package_note[^ ]* //g' Makefile
        make -j4 ; \
        make install_target INSTALL_ROOT=%{_builddir}/install-root )
%cmake -DCMAKE_BUILD_TYPE=Release \
                -DMAKE_TESTS=OFF \
                -DLRELEASE=lrelease-qt5 \
                -DLIBDIVECOMPUTER_INCLUDE_DIR=%{_builddir}/install-root/include \
                -DLIBGIT2_INCLUDE_DIR=%{_builddir}/install-root/include \
                -DLIBDIVECOMPUTER_LIBRARIES=%{_builddir}/install-root/lib/libdivecomputer.a \
                -DNO_PRINTING=OFF
%cmake_build

%install
mkdir -p %{buildroot}/%{_libdir}
( cd googlemaps/build ; make install_target INSTALL_ROOT=%{buildroot} )
%cmake_install
install subsurface.debug %{buildroot}%{_bindir}
install metainfo/subsurface.metainfo.xml %{buildroot}%{_datadir}/metainfo
desktop-file-install --dir=%{buildroot}/%{_datadir}/applications subsurface.desktop

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
%dir %{_datadir}/metainfo
%{_datadir}/metainfo/subsurface.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/subsurface-icon.*
%{_datadir}/subsurface/


%changelog
