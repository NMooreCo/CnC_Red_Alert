;
;	Command & Conquer Red Alert(tm)
;	Copyright 2025 Electronic Arts Inc.
;
;	This program is free software: you can redistribute it and/or modify
;	it under the terms of the GNU General Public License as published by
;	the Free Software Foundation, either version 3 of the License, or
;	(at your option) any later version.
;
;	This program is distributed in the hope that it will be useful,
;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;	GNU General Public License for more details.
;
;	You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <http://www.gnu.org/licenses/>.
;

;
;	Windows installer for the modern port (NSIS 3). Built by make-installer.sh, which
;	defines:
;
;	  BUILD_DIR    the Windows build (redalert.exe)
;	  SDL2_DLL     SDL2.dll
;	  SDL2_README  (optional) SDL's README-SDL.txt, which goes with SDL2.dll
;	  SOURCE_DIR   this repository (icon, README, license)
;	  SOURCE_URL   where the source code is (the GPL asks for that)
;	  VERSION      version text
;	  OUT_FILE     the installer to write
;	  DATA_DIR     (optional) the game data to put in the installer: REDALERT.MIX,
;	               CD1\MAIN.MIX (Allied disc), CD2\MAIN.MIX (Soviet disc)
;	  EULA         (with DATA_DIR) EA's license agreement for the data
;
;	Without DATA_DIR, the installer copies the data from the player's discs: real ones,
;	disc images opened in Windows, or folders holding a disc's files. For a silent install
;	(/S), give them as /ALLIED=path and /SOVIET=path.
;
;	The game is installed for the current user only, in a folder of theirs, as it writes its
;	settings and saved games next to its data.
;

Unicode true
Target amd64-unicode			; A 64-bit installer, like the game (Windows 11 is 64-bit only).
SetCompressor lzma

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "nsDialogs.nsh"

!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\RedAlertModernPort"
!define SETTINGS_KEY "Software\RedAlertModernPort"

Name "Command & Conquer Red Alert" "Command && Conquer Red Alert"
OutFile "${OUT_FILE}"
RequestExecutionLevel user
InstallDir "$LOCALAPPDATA\Programs\Red Alert"
InstallDirRegKey HKCU "${SETTINGS_KEY}" "InstallDir"
BrandingText "Red Alert modern port ${VERSION}"
ShowInstDetails show
ShowUninstDetails show
ManifestDPIAware true

VIProductVersion "3.3.0.0"
VIAddVersionKey "ProductName" "Command & Conquer Red Alert (modern port)"
VIAddVersionKey "FileDescription" "Command & Conquer Red Alert setup"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "Copyright 1996 Westwood Studios. Source code released by Electronic Arts under the GPL v3."

!ifndef DATA_DIR
Var AlliedDisc
Var SovietDisc
Var AlliedField
Var SovietField
!endif

!define MUI_ICON "${SOURCE_DIR}\CODE\REDALERT.ICO"
!define MUI_UNICON "${SOURCE_DIR}\CODE\REDALERT.ICO"
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC

!define MUI_WELCOMEPAGE_TITLE "Command && Conquer Red Alert"
!ifdef DATA_DIR
!define MUI_WELCOMEPAGE_TEXT "This installs Red Alert: a modern port of the game, built from the source code Electronic Arts released under the GPL, with the game data EA released as freeware in 2008.$\r$\n$\r$\nThe source code of this port is at:$\r$\n${SOURCE_URL}"
!else
!define MUI_WELCOMEPAGE_TEXT "This installs Red Alert: a modern port of the game, built from the source code Electronic Arts released under the GPL.$\r$\n$\r$\nThe game data comes from the game's discs (EA released them as freeware in 2008); you will be asked for them.$\r$\n$\r$\nThe source code of this port is at:$\r$\n${SOURCE_URL}"
!endif
!insertmacro MUI_PAGE_WELCOME

!ifdef DATA_DIR
!define MUI_PAGE_HEADER_TEXT "Game data license"
!define MUI_PAGE_HEADER_SUBTEXT "The game data comes with EA's license agreement."
!define MUI_LICENSEPAGE_TEXT_TOP "The game data (not the program) is covered by this agreement:"
!insertmacro MUI_PAGE_LICENSE "${EULA}"
!endif

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!ifndef DATA_DIR
Page custom Discs_Page Discs_Page_Leave
!endif
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_RUN "$INSTDIR\redalert.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Play Red Alert"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"


;-------------------------------------------------------------------------------------
;	The program.
;-------------------------------------------------------------------------------------
Section "Red Alert" SecProgram
	SectionIn RO
	SetOutPath "$INSTDIR"
	File "${BUILD_DIR}\redalert.exe"
	File "${SDL2_DLL}"
!ifdef SDL2_README
	File "/oname=README-SDL.txt" "${SDL2_README}"
!endif
	File "/oname=README.md" "${SOURCE_DIR}\README.md"
	File "/oname=LICENSE.md" "${SOURCE_DIR}\LICENSE.md"
	FileOpen $0 "$INSTDIR\SOURCE.txt" w
	FileWrite $0 "Red Alert, modern port ${VERSION}$\r$\n$\r$\nThe source code of this program, under the GNU General Public License v3$\r$\n(see LICENSE.md), is at:$\r$\n$\r$\n    ${SOURCE_URL}$\r$\n"
	FileClose $0

	WriteRegStr HKCU "${SETTINGS_KEY}" "InstallDir" "$INSTDIR"
	WriteUninstaller "$INSTDIR\uninstall.exe"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "Command & Conquer Red Alert (modern port)"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\redalert.exe"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "URLInfoAbout" "${SOURCE_URL}"
	WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
	WriteRegStr HKCU "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
	WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
	WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
SectionEnd


;-------------------------------------------------------------------------------------
;	The game data: in the installer, or copied from the discs.
;-------------------------------------------------------------------------------------
!ifdef DATA_DIR
Section "-Game data" SecData
	SetOutPath "$INSTDIR"
	File "${DATA_DIR}\REDALERT.MIX"
	File "/oname=EA License Agreement.rtf" "${EULA}"
	SetOutPath "$INSTDIR\CD1"
	File "${DATA_DIR}\CD1\MAIN.MIX"
	SetOutPath "$INSTDIR\CD2"
	File "${DATA_DIR}\CD2\MAIN.MIX"
SectionEnd
!else
Section "-Game data" SecData
	AddSize 960000
	${If} $AlliedDisc != ""
		Push $AlliedDisc
		Call Copy_Disc_Data
		CreateDirectory "$INSTDIR\CD1"
		DetailPrint "Copying the Allied disc's MAIN.MIX..."
		CopyFiles /SILENT "$AlliedDisc\MAIN.MIX" "$INSTDIR\CD1"
	${EndIf}
	${If} $SovietDisc != ""
		Push $SovietDisc
		Call Copy_Disc_Data
		CreateDirectory "$INSTDIR\CD2"
		DetailPrint "Copying the Soviet disc's MAIN.MIX..."
		CopyFiles /SILENT "$SovietDisc\MAIN.MIX" "$INSTDIR\CD2"
	${EndIf}
	; The files keep the read-only attribute they had on the disc.
	SetFileAttributes "$INSTDIR\REDALERT.MIX" NORMAL
	SetFileAttributes "$INSTDIR\CD1\MAIN.MIX" NORMAL
	SetFileAttributes "$INSTDIR\CD2\MAIN.MIX" NORMAL
	${IfNot} ${FileExists} "$INSTDIR\REDALERT.MIX"
		DetailPrint "The game data was not installed. Copy it into $INSTDIR (see README.md)."
	${EndIf}
SectionEnd

; Copies REDALERT.MIX from the disc on the stack (on the discs it is in INSTALL), unless the
; other disc already supplied it.
Function Copy_Disc_Data
	Exch $0
	${IfNot} ${FileExists} "$INSTDIR\REDALERT.MIX"
		DetailPrint "Copying REDALERT.MIX..."
		${If} ${FileExists} "$0\INSTALL\REDALERT.MIX"
			CopyFiles /SILENT "$0\INSTALL\REDALERT.MIX" "$INSTDIR"
		${Else}
			CopyFiles /SILENT "$0\REDALERT.MIX" "$INSTDIR"
		${EndIf}
	${EndIf}
	Pop $0
FunctionEnd
!endif


;-------------------------------------------------------------------------------------
;	Shortcuts.
;-------------------------------------------------------------------------------------
Section "Start menu shortcut" SecStartMenu
	SetOutPath "$INSTDIR"
	CreateShortcut "$SMPROGRAMS\Red Alert.lnk" "$INSTDIR\redalert.exe"
SectionEnd

Section "Desktop shortcut" SecDesktop
	SetOutPath "$INSTDIR"
	CreateShortcut "$DESKTOP\Red Alert.lnk" "$INSTDIR\redalert.exe"
SectionEnd

Section "-Size"
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKCU "${UNINSTALL_KEY}" "EstimatedSize" "$0"
SectionEnd


;-------------------------------------------------------------------------------------
;	Asking for the discs.
;-------------------------------------------------------------------------------------
!ifndef DATA_DIR
Function .onInit
	${GetParameters} $R0
	ClearErrors
	${GetOptions} $R0 "/ALLIED=" $AlliedDisc
	ClearErrors
	${GetOptions} $R0 "/SOVIET=" $SovietDisc
FunctionEnd

Function Discs_Page
	!insertmacro MUI_HEADER_TEXT "Game discs" "Where should the game data be copied from?"
	nsDialogs::Create 1018
	Pop $0
	${NSD_CreateLabel} 0 0 100% 52u "Red Alert came on two discs: the Allied disc (CD 1) and the Soviet disc (CD 2), each with one of the campaigns. Insert a disc, or open a disc image (.iso) by double-clicking it, or use a folder that holds a disc's files, and choose it below. Leave a field empty to skip that disc."
	Pop $0

	${NSD_CreateLabel} 0 58u 100% 10u "Allied disc (CD 1):"
	Pop $0
	${NSD_CreateDirRequest} 0 70u 245u 13u "$AlliedDisc"
	Pop $AlliedField
	${NSD_CreateBrowseButton} 250u 70u 50u 13u "Browse..."
	Pop $0
	nsDialogs::SetUserData $0 $AlliedField
	${NSD_OnClick} $0 Browse_For_Disc

	${NSD_CreateLabel} 0 92u 100% 10u "Soviet disc (CD 2):"
	Pop $0
	${NSD_CreateDirRequest} 0 104u 245u 13u "$SovietDisc"
	Pop $SovietField
	${NSD_CreateBrowseButton} 250u 104u 50u 13u "Browse..."
	Pop $0
	nsDialogs::SetUserData $0 $SovietField
	${NSD_OnClick} $0 Browse_For_Disc

	nsDialogs::Show
FunctionEnd

Function Browse_For_Disc
	Pop $0
	nsDialogs::GetUserData $0
	Pop $1
	${NSD_GetText} $1 $2
	nsDialogs::SelectFolderDialog "Choose the disc, or the folder holding its files" "$2"
	Pop $2
	${If} $2 != "error"
		${NSD_SetText} $1 $2
	${EndIf}
FunctionEnd

; Removes a trailing backslash ("E:\" becomes "E:").
!macro Trim_Path VAR
	StrCpy $0 ${VAR} 1 -1
	${If} $0 == "\"
		StrCpy ${VAR} ${VAR} -1
	${EndIf}
!macroend

Function Discs_Page_Leave
	${NSD_GetText} $AlliedField $AlliedDisc
	${NSD_GetText} $SovietField $SovietDisc
	!insertmacro Trim_Path $AlliedDisc
	!insertmacro Trim_Path $SovietDisc

	${If} $AlliedDisc != ""
	${AndIfNot} ${FileExists} "$AlliedDisc\MAIN.MIX"
		MessageBox MB_OK|MB_ICONEXCLAMATION "There is no MAIN.MIX in $AlliedDisc. Choose the Allied disc itself (or the folder that holds its files)."
		Abort
	${EndIf}
	${If} $SovietDisc != ""
	${AndIfNot} ${FileExists} "$SovietDisc\MAIN.MIX"
		MessageBox MB_OK|MB_ICONEXCLAMATION "There is no MAIN.MIX in $SovietDisc. Choose the Soviet disc itself (or the folder that holds its files)."
		Abort
	${EndIf}

	; REDALERT.MIX is on both discs, in their INSTALL folder.
	StrCpy $1 "no"
	${If} $AlliedDisc != ""
		${If} ${FileExists} "$AlliedDisc\INSTALL\REDALERT.MIX"
		${OrIf} ${FileExists} "$AlliedDisc\REDALERT.MIX"
			StrCpy $1 "yes"
		${EndIf}
	${EndIf}
	${If} $SovietDisc != ""
		${If} ${FileExists} "$SovietDisc\INSTALL\REDALERT.MIX"
		${OrIf} ${FileExists} "$SovietDisc\REDALERT.MIX"
			StrCpy $1 "yes"
		${EndIf}
	${EndIf}

	${If} $AlliedDisc == ""
	${AndIf} $SovietDisc == ""
		MessageBox MB_YESNO|MB_ICONQUESTION "Install without the game data? The game can't run until REDALERT.MIX and the discs' MAIN.MIX files are copied into its folder (see README.md)." IDYES +2
		Abort
	${ElseIf} $1 == "no"
		MessageBox MB_OK|MB_ICONEXCLAMATION "REDALERT.MIX was not found (on a disc it is in the INSTALL folder). Choose the discs themselves, or folders that hold all their files."
		Abort
	${EndIf}
FunctionEnd
!endif


;-------------------------------------------------------------------------------------
;	Uninstalling. Saved games and settings are only deleted if the player says so; other
;	files they put in the folder (mods...) stay.
;-------------------------------------------------------------------------------------
Section "Uninstall"
	Delete "$INSTDIR\redalert.exe"
	Delete "$INSTDIR\SDL2.dll"
	Delete "$INSTDIR\README-SDL.txt"
	Delete "$INSTDIR\README.md"
	Delete "$INSTDIR\LICENSE.md"
	Delete "$INSTDIR\SOURCE.txt"
	Delete "$INSTDIR\EA License Agreement.rtf"
	Delete "$INSTDIR\REDALERT.MIX"
	Delete "$INSTDIR\CD1\MAIN.MIX"
	Delete "$INSTDIR\CD2\MAIN.MIX"
	RMDir "$INSTDIR\CD1"
	RMDir "$INSTDIR\CD2"
	Delete "$INSTDIR\uninstall.exe"
	Delete "$SMPROGRAMS\Red Alert.lnk"
	Delete "$DESKTOP\Red Alert.lnk"
	DeleteRegKey HKCU "${UNINSTALL_KEY}"
	DeleteRegKey HKCU "${SETTINGS_KEY}"

	${If} ${FileExists} "$INSTDIR\SAVEGAME.*"
	${OrIf} ${FileExists} "$INSTDIR\REDALERT.INI"
		MessageBox MB_YESNO|MB_ICONQUESTION "Also delete your saved games and settings?" /SD IDNO IDNO keep
		Delete "$INSTDIR\SAVEGAME.*"
		Delete "$INSTDIR\REDALERT.INI"
		Delete "$INSTDIR\HALLFAME.DAT"
		Delete "$INSTDIR\*_VTX.PAL"
	keep:
	${EndIf}
	RMDir "$INSTDIR"
SectionEnd
