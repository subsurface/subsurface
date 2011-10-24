Name: 		subsurface
Summary: 	Simple Dive Log Program
Version: 	1.1
Release:    	5
License:	GPLv2
URL:		http://subsurface.hohndel.org
Source0:        subsurface-1.1.tar.gz
BuildRequires:  pkgconfig gtk2-devel glib2-devel libxml2-devel libdivecomputer0-devel 
%if 0%{?suse_version}
BuildRequires:  gconf2-devel update-desktop-files
%else
BuildRequires:  GConf2-devel
%endif

Group:		Productivity/Other

%description
subsurface is a simple dive log program written in C

%prep
%setup -q

%build
make

%clean
rm -rf %{buildroot}

%install
make install prefix=%buildroot/usr
%if 0%{?suse_version}
%suse_update_desktop_file -r %{name} Utility SyncUtility 
%endif
rm %{buildroot}/%{_datadir}/icons/hicolor/icon-theme.cache

%files 
%defattr(-,root,root)
%{_bindir}/subsurface
%{_datadir}/applications/subsurface.desktop
%{_datadir}/icons/hicolor/scalable/apps/subsurface.svg
%{_datadir}/man/man1/subsurface.1.gz


%changelog
