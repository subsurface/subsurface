# this installer creator needs to be run with
# makensis subsurface.nsi
#
# it assumes that packaging/windows/dll is a symlink to
# the directory in which the required Windows DLLs are installed
# (in my case that's /usr/i686-w64-mingw32/sys-root/mingw/bin)
#
# define the name of the installer
outfile "subsurface-installer.exe"
Name subsurface

# some data for the package to identify itself
VIProductVersion "1.1.9.0"
VIAddVersionKey ProductName subsurface
VIAddVersionKey FileDescription "subsurface diving log program"
VIAddVersionKey LegalCopyright "GPL v.2"
VIAddVersionKey ProductVersion "1.1"
VIAddVersionKey FileVersion "1.1"

# icon to use for the installer
Icon .\subsurface.ico

# the installer needs to be run with admin privileges
RequestExecutionLevel admin

# pop up a little dialog that tells the user that we're about to
# install subsurface
Function .onInit
  MessageBox MB_YESNO "This will install subsurface. Do you wish to continue?" IDYES gogogo
    Abort
  gogogo:
FunctionEnd
 
# define the directory to install to, the desktop in this case as specified  
# by the predefined $DESKTOP variable
installDir "$PROGRAMFILES\subsurface"

# default section
Section

# define the output path for this file
setOutPath $INSTDIR

SetShellVarContext all

# create directory in the Start menu
CreateDirectory "$SMPROGRAMS\subsurface"

# create Start menu shortcut
createShortCut "$SMPROGRAMS\subsurface\subsurface.lnk" "$INSTDIR\subsurface.exe"

#create uninstaller and corresponding shortcut in Start menu
writeUninstaller "$INSTDIR\subsurface-uninstall.exe"
createShortCut "$SMPROGRAMS\subsurface\uninstall-subsurface.lnk" "$INSTDIR\subsurface-uninstall.exe"
 
# define what to install and place it in the output path
file /oname=subsurface.exe ../../subsurface.exe
file /oname=subsurface.ico subsurface.ico
file /oname=subsurface.svg ../../subsurface.svg
file /oname=libatk-1.0-0.dll dll/libatk-1.0-0.dll
file /oname=libcairo-2.dll dll/libcairo-2.dll
file /oname=libdivecomputer-0.dll dll\libdivecomputer-0.dll
file /oname=libffi-5.dll dll\libffi-5.dll
file /oname=libfontconfig-1.dll dll\libfontconfig-1.dll
file /oname=libfreetype-6.dll dll\libfreetype-6.dll
file /oname=libgdk_pixbuf-2.0-0.dll dll\libgdk_pixbuf-2.0-0.dll
file /oname=libgdk-win32-2.0-0.dll dll\libgdk-win32-2.0-0.dll
file /oname=libgio-2.0-0.dll dll\libgio-2.0-0.dll
file /oname=libglib-2.0-0.dll dll\libglib-2.0-0.dll
file /oname=libgmodule-2.0-0.dll dll\libgmodule-2.0-0.dll
file /oname=libgobject-2.0-0.dll dll\libgobject-2.0-0.dll
file /oname=libgthread-2.0-0.dll dll\libgthread-2.0-0.dll
file /oname=libgtk-win32-2.0-0.dll dll\libgtk-win32-2.0-0.dll
file /oname=libintl-8.dll dll\libintl-8.dll
file /oname=libjasper-1.dll dll\libjasper-1.dll
file /oname=libjpeg-8.dll dll\libjpeg-8.dll
file /oname=libpango-1.0-0.dll dll\libpango-1.0-0.dll
file /oname=libpangocairo-1.0-0.dll dll\libpangocairo-1.0-0.dll
file /oname=libpangoft2-1.0-0.dll dll\libpangoft2-1.0-0.dll
file /oname=libpangowin32-1.0-0.dll dll\libpangowin32-1.0-0.dll
file /oname=libpixman-1-0.dll dll\libpixman-1-0.dll
file /oname=libpng15-15.dll dll\libpng15-15.dll
file /oname=libtiff-3.dll dll\libtiff-3.dll
file /oname=libxml2-2.dll dll\libxml2-2.dll
file /oname=pthreadGC2.dll dll\pthreadGC2.dll
file /oname=zlib1.dll dll\zlib1.dll
sectionEnd

section "uninstall"
  SetShellVarContext all
  delete "$INSTDIR\subsurface-uninstall.exe"
  delete "$INSTDIR\*.*"
  RMDir  "$INSTDIR"
  delete "$SMPROGRAMS\subsurface\uninstall-subsurface.lnk"
  delete "$SMPROGRAMS\subsurface\subsurface.lnk"
  RMDir  "$SMPROGRAMS\subsurface"

  MessageBox MB_YESNO "Do you wish to keep subsurface's registry settings?" IDYES end
  DeleteRegKey HKCU "SOFTWARE\subsurface"
  end:
sectionEnd
