%define __strip %{_mingw32_strip}
%define __objdump %{_mingw32_objdump}
%define _use_internal_dependency_generator 0
%define __find_requires %{_mingw32_findrequires}
%define __find_provides %{_mingw32_findprovides}
%define __os_install_post %{_mingw32_debug_install_post} \
                          %{_mingw32_install_post}


Name: 		mingw32-subsurface
Summary: 	Simple Dive Log Program
Version: 	1.1
Release:    	5
License:	GPLv2
URL:		http://subsurface.hohndel.org
Source0:        subsurface-1.1.tar.gz
BuildArch:      noarch
BuildRequires:  mingw32-cross-pkg-config mingw32-cross-gcc
BuildRequires:  mingw32-gtk2-devel mingw32-glib2-devel mingw32-libxml2-devel
BuildRequires:  mingw32-libdivecomputer0-devel mingw32-gconf2-devel
BuildRequires:  mingw32-pthreads-devel
BuildRequires:  mingw32-gtk2 mingw32-glib2 mingw32-libxml2
BuildRequires:  mingw32-libdivecomputer0 mingw32-gconf2
BuildRequires:  mingw32-pthreads mingw32-zlib

Group:		Productivity/Other

%description
subsurface is a simple dive log program written in C

%{_mingw32_debug_package}

%prep
%setup -q -n subsurface-1.1

%build
make CC=%{_mingw32_target}-gcc PKGCONFIG=%{_mingw32_target}-pkg-config XML2CONFIG=%{_mingw32_bindir}/xml2-config NAME=subsurface.exe

%clean
#rm -rf %{buildroot}

%install
mkdir -p $RPM_BUILD_ROOT/%{_mingw32_bindir}
mkdir -p $RPM_BUILD_ROOT/%{_mingw32_datadir}
install -m 755 subsurface.exe $RPM_BUILD_ROOT/%{_mingw32_bindir}/subsurface.exe
install -m 644 subsurface-icon.svg $RPM_BUILD_ROOT/%{_mingw32_datadir}/subsurface-icon.svg
# this seems like a really ugly hack
install -m 755 %{_mingw32_bindir}/libdivecomputer-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libdivecomputer-0.dll
install -m 755 %{_mingw32_bindir}/libcairo-2.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libcairo-2.dll
install -m 755 %{_mingw32_bindir}/libgconf-2-4.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgconf-2-4.dll
install -m 755 %{_mingw32_bindir}/libgdk-win32-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgdk-win32-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libglib-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libglib-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libgtk-win32-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgtk-win32-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libpango-1.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpango-1.0-0.dll
install -m 755 %{_mingw32_bindir}/libpangocairo-1.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpangocairo-1.0-0.dll
install -m 755 %{_mingw32_bindir}/pthreadGC2.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/pthreadGC2.dll
install -m 755 %{_mingw32_bindir}/libxml2-2.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libxml2-2.dll
install -m 755 %{_mingw32_bindir}/libfontconfig-1.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libfontconfig-1.dll
install -m 755 %{_mingw32_bindir}/libfreetype-6.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libfreetype-6.dll
install -m 755 %{_mingw32_bindir}/libpixman-1-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpixman-1-0.dll
install -m 755 %{_mingw32_bindir}/libpng15-15.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpng15-15.dll
install -m 755 %{_mingw32_bindir}/zlib1.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/zlib1.dll
install -m 755 %{_mingw32_bindir}/libintl-8.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libintl-8.dll
install -m 755 %{_mingw32_bindir}/libgmodule-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgmodule-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libgdk_pixbuf-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgdk_pixbuf-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libgobject-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgobject-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libgio-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgio-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libjasper-1.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libjasper-1.dll
install -m 755 %{_mingw32_bindir}/libgthread-2.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libgthread-2.0-0.dll
install -m 755 %{_mingw32_bindir}/libffi-5.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libffi-5.dll
install -m 755 %{_mingw32_bindir}/libjpeg-8.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libjpeg-8.dll
install -m 755 %{_mingw32_bindir}/libtiff-3.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libtiff-3.dll
install -m 755 %{_mingw32_bindir}/libpangoft2-1.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpangoft2-1.0-0.dll
install -m 755 %{_mingw32_bindir}/libpangowin32-1.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libpangowin32-1.0-0.dll
install -m 755 %{_mingw32_bindir}/libatk-1.0-0.dll $RPM_BUILD_ROOT/%{_mingw32_bindir}/libatk-1.0-0.dll

%files 
%defattr(-,root,root)
%{_mingw32_bindir}/subsurface.exe
%{_mingw32_bindir}/libdivecomputer-0.dll
%{_mingw32_bindir}/libcairo-2.dll
%{_mingw32_bindir}/libgconf-2-4.dll
%{_mingw32_bindir}/libgdk-win32-2.0-0.dll
%{_mingw32_bindir}/libglib-2.0-0.dll
%{_mingw32_bindir}/libgtk-win32-2.0-0.dll
%{_mingw32_bindir}/libpango-1.0-0.dll
%{_mingw32_bindir}/libpangocairo-1.0-0.dll
%{_mingw32_bindir}/pthreadGC2.dll
%{_mingw32_bindir}/libxml2-2.dll
%{_mingw32_bindir}/libfontconfig-1.dll
%{_mingw32_bindir}/libfreetype-6.dll
%{_mingw32_bindir}/libpixman-1-0.dll
%{_mingw32_bindir}/libpng15-15.dll
%{_mingw32_bindir}/zlib1.dll
%{_mingw32_bindir}/libintl-8.dll
%{_mingw32_bindir}/libgmodule-2.0-0.dll
%{_mingw32_bindir}/libgdk_pixbuf-2.0-0.dll
%{_mingw32_bindir}/libgobject-2.0-0.dll
%{_mingw32_bindir}/libgio-2.0-0.dll
%{_mingw32_bindir}/libjasper-1.dll
%{_mingw32_bindir}/libgthread-2.0-0.dll
%{_mingw32_bindir}/libffi-5.dll
%{_mingw32_bindir}/libjpeg-8.dll
%{_mingw32_bindir}/libtiff-3.dll
%{_mingw32_bindir}/libpangoft2-1.0-0.dll
%{_mingw32_bindir}/libpangowin32-1.0-0.dll
%{_mingw32_bindir}/libatk-1.0-0.dll

%{_mingw32_datadir}/subsurface-icon.svg


%changelog
