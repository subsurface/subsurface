#
# Subsurface NSIS installer script
#
# This installer creator needs to be run with:
# makensis subsurface.nsi
#
# It assumes that packaging/windows/dll is a symlink to
# the directory in which the required Windows DLLs are installed
# (in my case that's /usr/i686-w64-mingw32/sys-root/mingw/bin)
#

#--------------------------------
# Include Modern UI

    !include "MUI2.nsh"

#--------------------------------
# General

    !define VERSION "1.2"

    # Installer name and filename
    Name "Subsurface"
    Caption "Subsurface ${VERSION} Setup"
    OutFile "subsurface-${VERSION}.exe"

    # Icon to use for the installer
    !define MUI_ICON "subsurface.ico"

    # Default installation folder
    InstallDir "$PROGRAMFILES\Subsurface"

    # Get installation folder from registry if available
    InstallDirRegKey HKCU "Software\Subsurface" ""

    # Request application privileges
    RequestExecutionLevel user

#--------------------------------
# Version information

    VIProductVersion "${VERSION}"
    VIAddVersionKey "ProductName" "Subsurface"
    VIAddVersionKey "FileDescription" "Subsurface - an open source dive log program."
    VIAddVersionKey "FileVersion" "${VERSION}"
    VIAddVersionKey "LegalCopyright" "GPL v.2"
    VIAddVersionKey "ProductVersion" "${VERSION}"

#--------------------------------
# Settings

    # Show a warn on aborting installation
    !define MUI_ABORTWARNING

    # Defines the target start menu folder
    !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
    !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\Subsurface"
    !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

#--------------------------------
# Variables

    Var StartMenuFolder

#--------------------------------
# Pages

    # Installer pages
    !insertmacro MUI_PAGE_LICENSE "..\..\gpl-2.0.txt"
    !insertmacro MUI_PAGE_DIRECTORY
    !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
    !insertmacro MUI_PAGE_INSTFILES

    # Uninstaller pages
    !insertmacro MUI_UNPAGE_CONFIRM
    !insertmacro MUI_UNPAGE_INSTFILES

#--------------------------------
# Languages

    !insertmacro MUI_LANGUAGE "English"

#--------------------------------
# Default installer section

Section

    # Installation path
    SetOutPath "$INSTDIR"

    # Files to include in installer
    file /oname=subsurface.exe ..\..\subsurface.exe
    file /oname=subsurface.ico subsurface.ico
    file /oname=subsurface.svg ..\..\subsurface.svg
    file /oname=libatk-1.0-0.dll dll\libatk-1.0-0.dll
    file /oname=libcairo-2.dll dll\libcairo-2.dll
    file /oname=libdivecomputer-0.dll dll\libdivecomputer-0.dll
    file /oname=libffi-6.dll dll\libffi-6.dll
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
    file /oname=libjpeg-62.dll dll\libjpeg-62.dll
    file /oname=libpango-1.0-0.dll dll\libpango-1.0-0.dll
    file /oname=libpangocairo-1.0-0.dll dll\libpangocairo-1.0-0.dll
    file /oname=libpangoft2-1.0-0.dll dll\libpangoft2-1.0-0.dll
    file /oname=libpangowin32-1.0-0.dll dll\libpangowin32-1.0-0.dll
    file /oname=libpixman-1-0.dll dll\libpixman-1-0.dll
    file /oname=libpng15-15.dll dll\libpng15-15.dll
    file /oname=libtiff-3.dll dll\libtiff-3.dll
    file /oname=libxml2-2.dll dll\libxml2-2.dll
    file /oname=libxslt-1.dll dll\libxslt-1.dll
    file /oname=pthreadGC2.dll dll\pthreadGC2.dll
    file /oname=zlib1.dll dll\zlib1.dll
    file /oname=libusb-1.0.dll dll\libusb-1.0.dll
    file /oname=SuuntoSDM.xslt ..\..\xslt\SuuntoSDM.xslt
    file /oname=jdivelog2subsurface.xslt ..\..\xslt\jdivelog2subsurface.xslt
    file /oname=iconv.dll dll\iconv.dll

    # Store installation folder in registry
    WriteRegStr HKCU "Software\Subsurface" "" $INSTDIR

    # Create shortcuts
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Subsurface.lnk" "$INSTDIR\subsurface.exe"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall Subsurface.lnk" "$INSTDIR\Uninstall.exe"
    !insertmacro MUI_STARTMENU_WRITE_END

    # Create the uninstaller
    WriteUninstaller "$INSTDIR\Uninstall.exe"

SectionEnd

#--------------------------------
# Uninstaller section

Section "Uninstall"

    # Delete installed files
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\*.xslt"
    Delete "$INSTDIR\subsurface.exe"
    Delete "$INSTDIR\subsurface.ico"
    Delete "$INSTDIR\subsurface.svg"
    RMDir "$INSTDIR"

    # Remove shortcuts
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\Subsurface.lnk"
    Delete "$SMPROGRAMS\$StartMenuFolder\Uninstall Subsurface.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"

    # Remove registry entries
    DeleteRegKey /ifempty HKCU "Software\Subsurface"

SectionEnd
