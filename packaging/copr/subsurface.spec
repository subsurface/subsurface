#
# Fedora COPR spec file for package subsurface
#
# Copyright (c) 2014-2026 Dirk Hohndel
#

%define latestVersion 0.0.0.0
%define packageRevision 1

Name:           subsurface
Version:	%latestVersion
Release:        %packageRevision%{?dist}

Summary:        SUMMARY

License:        GPL v2
Url:            http://subsurface-divelog.org

Source:		subsurface-%latestVersion-%packageRevision.orig.tar.xz

Group:          Productivity/Other

BuildRequires:  desktop-file-utils
BuildRequires:  fdupes
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  asciidoctor
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  cmake
BuildRequires:  libzip-devel
BuildRequires:  libxml2-devel
BuildRequires:  libxslt-devel
BuildRequires:  libssh2-devel
BuildRequires:  libcurl-devel
BuildRequires:  pkgconfig(libgit2)
BuildRequires:  libmtp-devel
BuildRequires:  LibRaw-devel
BuildRequires:  netpbm-devel
BuildRequires:  openssl-devel
BuildRequires:  libsqlite3x-devel
BuildRequires:  libusbx-devel
BuildRequires:  bluez-libs-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtbase-private-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtbase-mysql
BuildRequires:  qt6-qtbase-postgresql
BuildRequires:  qt6-qtbase-ibase
BuildRequires:  qt6-qtbase-odbc
BuildRequires:  qt6-qtconnectivity-devel
BuildRequires:  qt6-qtlocation-devel
BuildRequires:  qt6-qtpositioning-devel
BuildRequires:  qt6-qt5compat-devel
BuildRequires:  libappstream-glib

Recommends:     qt6-qttranslations

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
        qmake6 ../googlemaps.pro ; \
        # on Fedora 36 and newer we get the package_notes that break the build - let's rip them out
        sed -i 's/-Wl[^ ]*package_note[^ ]* //g' Makefile
        make -j4 ; \
        make install_target INSTALL_ROOT=%{_builddir}/install-root )
# QLiteHtml is not packaged for Fedora, so build it from the sources that
# make-package.sh vendored into the tarball, same as we do for the other
# private dependencies above. It only supports being configured in its own
# source tree (no separate build directory).
( cd qlitehtml ; \
        cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_builddir}/install-root . ; \
        make %{?_smp_mflags} ; \
        make install )
%cmake -DCMAKE_BUILD_TYPE=Release \
                -DBUILD_WITH_QT6=ON \
                -DLRELEASE=lrelease-qt6 \
                -DLIBDIVECOMPUTER_INCLUDE_DIR=%{_builddir}/install-root/include \
                -DLIBGIT2_INCLUDE_DIR=%{_builddir}/install-root/include \
                -DLIBDIVECOMPUTER_LIBRARIES=%{_builddir}/install-root/lib/libdivecomputer.a \
                -DCMAKE_PREFIX_PATH=%{_builddir}/install-root \
                -DNO_PRINTING=OFF \
                -DBUILD_DOCS=ON
%cmake_build

%install
mkdir -p %{buildroot}/%{_libdir}
( cd googlemaps/build ; make install_target INSTALL_ROOT=%{buildroot} )
# QLiteHtml isn't a Fedora package, so ship the shared library we built
# above alongside subsurface rather than relying on the system to have it.
cp -a %{_builddir}/install-root/lib64/libqlitehtml.so* %{buildroot}%{_libdir}/
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
%{_libdir}/qt6/plugins/geoservices/libqtgeoservices_googlemaps.so
%{_libdir}/libqlitehtml.so*
%{_datadir}/applications/subsurface.desktop
%dir %{_datadir}/metainfo
%{_datadir}/metainfo/subsurface.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/subsurface-icon.*
%{_datadir}/subsurface/


%changelog
