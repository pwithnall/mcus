;Based on:
;NSIS Modern User Interface
;Welcome/Finish Page Example Script
;Written by Joost Verburg

;--------------------------------
;Include Modern UI and registerExtension

	!include "MUI2.nsh"
	!include "registerExtension.nsh"

;--------------------------------
;General

	;General defines
	!define SHORT_NAME "MCUS"
	!define SHORT_NAME_ "mcus"
	!define APP_NAME "${SHORT_NAME_}.exe"
	!define LONG_NAME "Microcontroller Simulator"
	!define AUTHOR "Philip Withnall"
	!define WEBSITE "http://mcus.tecnocode.co.uk/"
	!define VERSION "0.1.1"
	!define VERSION_MAJOR "0"
	!define VERSION_MINOR "1"
	!define VERSION_REVISION "1"

	;Name and file
	Name "${SHORT_NAME}"
	OutFile "${SHORT_NAME_}-${VERSION}.exe"

	;Default installation folder
	InstallDir "$PROGRAMFILES\${SHORT_NAME}"

	;Get installation folder from registry if available
	InstallDirRegKey HKLM "Software\${SHORT_NAME}" ""

	;Request application privileges for Windows Vista
	RequestExecutionLevel user

;--------------------------------
;Interface Settings

	!define MUI_ABORTWARNING

;--------------------------------
;Pages

	!insertmacro MUI_PAGE_WELCOME
	!insertmacro MUI_PAGE_LICENSE "../COPYING"
	!insertmacro MUI_PAGE_DIRECTORY
	!insertmacro MUI_PAGE_INSTFILES
	!insertmacro MUI_PAGE_FINISH

	!insertmacro MUI_UNPAGE_WELCOME
	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES
	!insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

	!insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "Install"

	;Install files
	SetOutPath "$INSTDIR"
	File /r "GTK2-Runtime\bin"
	File /r "GTK2-Runtime\etc"
	File /r "GTK2-Runtime\gtk2-runtime"
	File /r "GTK2-Runtime\lib"
	File /r "GTK2-Runtime\share"

	;Install Start Menu shortcut
	CreateShortCut "$SMPROGRAMS\${SHORT_NAME}.lnk" "$INSTDIR\lib\${APP_NAME}" "" "$INSTDIR\lib\${APP_NAME}" 0

	;Store installation folder
	WriteRegStr HKLM "Software\${SHORT_NAME}" "" $INSTDIR

	;Create uninstaller
	WriteUninstaller "$INSTDIR\uninstall.exe"

	;Add to Add/Remove Programs
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"DisplayName" "${SHORT_NAME}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"DisplayIcon" "$INSTDIR\lib\${APP_NAME},0"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"Publisher" "${AUTHOR}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"URLInfoAbout" "${WEBSITE}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"DisplayVersion" "${VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"VersionMajor" "${VERSION_MAJOR}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"VersionMinor" "${VERSION_MINOR}"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"NoModify" "1"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}" \
		"NoRepair" "1"

	;Register file association
	${registerExtension} "$INSTDIR\lib\${APP_NAME}" ".asm" "OCR Assembly Code"

SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

	;Remove installed files
	Delete "$INSTDIR\uninstall.exe"
	RMDir /r "$INSTDIR"

	;Remove Start Menu shortcut
	Delete "$SMPROGRAMS\${SHORT_NAME}.lnk"

	;Remove registry keys
	DeleteRegKey /ifempty HKLM "Software\${SHORT_NAME}"

	;Remove from Add/Remove programs
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${SHORT_NAME}"

	;Unregister file association
	${unregisterExtension} ".asm" "OCR Assembly Code"

SectionEnd

