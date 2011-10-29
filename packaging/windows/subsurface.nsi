
# define the name of the installer
outfile "subsurface-installer.exe"
Name subsurface

VIProductVersion "1.1.0.0"
VIAddVersionKey ProductName subsurface
VIAddVersionKey ProductVersion "1.1"
VIAddVersionKey FileVersion "1.1"

Icon ..\share\subsurface.ico


RequestExecutionLevel admin

Function .onInit
  MessageBox MB_YESNO "This will install subsurface. Do you wish to continue?" IDYES gogogo
    Abort
  gogogo:
FunctionEnd
 
# define the directory to install to, the desktop in this case as specified  
# by the predefined $DESKTOP variable
installDir "$DESKTOP\subsurface"

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
file subsurface.exe
file libatk-1.0-0.dll
file libcairo-2.dll
file libdivecomputer-0.dll
file libffi-5.dll
file libfontconfig-1.dll
file libfreetype-6.dll
file libgdk_pixbuf-2.0-0.dll
file libgdk-win32-2.0-0.dll
file libgio-2.0-0.dll
file libglib-2.0-0.dll
file libgmodule-2.0-0.dll
file libgobject-2.0-0.dll
file libgthread-2.0-0.dll
file libgtk-win32-2.0-0.dll
file libintl-8.dll
file libjasper-1.dll
file libjpeg-8.dll
file libpango-1.0-0.dll
file libpangocairo-1.0-0.dll
file libpangoft2-1.0-0.dll
file libpangowin32-1.0-0.dll
file libpixman-1-0.dll
file libpng15-15.dll
file libtiff-3.dll
file libxml2-2.dll
file pthreadGC2.dll
file zlib1.dll
file /oname=subsurface.ico ..\\share\\subsurface.ico
file /oname=subsurface.bmp ..\\share\\subsurface.bmp
file /oname=subsurface.svg ..\\share\\subsurface.svg
 

sectionEnd

section "uninstall"
  SetShellVarContext all
  delete "$INSTDIR\subsurface-uninstall.exe"
  delete "$INSTDIR\*.*"
  RMDir  "$INSTDIR"
  delete "$SMPROGRAMS\subsurface\uninstall-subsurface.lnk"
  delete "$SMPROGRAMS\subsurface\subsurface.lnk"
  RMDir  "$SMPROGRAMS\subsurface"
sectionEnd

