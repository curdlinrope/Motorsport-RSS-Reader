; MotorsportRSS Windows Installer Script
; For use with NSIS (Nullsoft Scriptable Install System)

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Basic application information
Name "MotorsportRSS"
OutFile "MotorsportRSS-Installer.exe"
Unicode True

; Default installation directory
InstallDir "$PROGRAMFILES64\MotorsportRSS"

; Request application privileges
RequestExecutionLevel admin

; Interface Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\win.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_RIGHT
!define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\win.bmp"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

Section "Install"
  SetOutPath "$INSTDIR"
  
  ; Copy the application files
  File "MotorsportRSS.exe"
  File "Qt5Core.dll"
  File "Qt5Gui.dll"
  File "Qt5Widgets.dll"
  File "Qt5Svg.dll"
  File "Qt5Network.dll"
  File "Qt5Xml.dll"
  
  ; Create platforms directory and copy plugin
  CreateDirectory "$INSTDIR\platforms"
  SetOutPath "$INSTDIR\platforms"
  File "platforms\qwindows.dll"
  
  ; Create resources directory and copy resources
  CreateDirectory "$INSTDIR\resources"
  SetOutPath "$INSTDIR\resources"
  File /r "resources\*.*"
  
  ; Reset output path
  SetOutPath "$INSTDIR"
  
  ; Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  
  ; Create shortcuts
  CreateDirectory "$SMPROGRAMS\MotorsportRSS"
  CreateShortcut "$SMPROGRAMS\MotorsportRSS\MotorsportRSS.lnk" "$INSTDIR\MotorsportRSS.exe"
  CreateShortcut "$SMPROGRAMS\MotorsportRSS\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\MotorsportRSS.lnk" "$INSTDIR\MotorsportRSS.exe"
  
  ; Write registry information
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "DisplayName" "MotorsportRSS"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "InstallLocation" "$\"$INSTDIR$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "Publisher" "MotorsportRSS Team"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "DisplayVersion" "1.0.0"
  
  ; Calculate installed size
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS" "EstimatedSize" "$0"
SectionEnd

Section "Uninstall"
  ; Remove files and directories
  Delete "$INSTDIR\MotorsportRSS.exe"
  Delete "$INSTDIR\Qt5Core.dll"
  Delete "$INSTDIR\Qt5Gui.dll"
  Delete "$INSTDIR\Qt5Widgets.dll"
  Delete "$INSTDIR\Qt5Svg.dll"
  Delete "$INSTDIR\Qt5Network.dll"
  Delete "$INSTDIR\Qt5Xml.dll"
  Delete "$INSTDIR\platforms\qwindows.dll"
  RMDir "$INSTDIR\platforms"
  RMDir /r "$INSTDIR\resources"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  
  ; Remove shortcuts
  Delete "$SMPROGRAMS\MotorsportRSS\MotorsportRSS.lnk"
  Delete "$SMPROGRAMS\MotorsportRSS\Uninstall.lnk"
  RMDir "$SMPROGRAMS\MotorsportRSS"
  Delete "$DESKTOP\MotorsportRSS.lnk"
  
  ; Remove registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\MotorsportRSS"
SectionEnd 