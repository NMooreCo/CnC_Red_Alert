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
;	  SEVENZIP_DIR (optional, without DATA_DIR) 7-Zip's 7z.exe, 7z.dll and License.txt, for
;	               the option of downloading the data
;	  RADL_DIR     (with SEVENZIP_DIR) the download plugin, radl.dll (radl.c)
;
;	Without DATA_DIR, the installer gets the data from EA's freeware release of the discs:
;	it downloads it (from the Internet Archive; the files are checked against their SHA-256
;	digests, then unpacked with 7-Zip), or copies it from the player's discs (real ones,
;	disc images opened in Windows, or folders holding a disc's files).
;
;	Silent install (/S): /DATA=download, or /ALLIED=path and /SOVIET=path for discs; without
;	either, no data is installed. /MIRROR=URL downloads the .rar files from another place.
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
; EA's freeware release of the discs, as kept by the Internet Archive.
!define DATA_URL "https://archive.org/details/official-cn-ctiberian-sun_202510"

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
Var DataSource			; download, discs or none
Var AlliedDisc
Var SovietDisc
Var Mirror
Var DownloadRadio
Var DiscsRadio
Var NoneRadio
Var AlliedField
Var SovietField
Var AlliedBrowse
Var SovietBrowse

; EA's freeware release: two RAR archives, each holding a disc image and EA's license.
!define DATA_MIRROR "https://archive.org/download/official-cn-ctiberian-sun_202510/"
!define ALLIED_RAR "RedAlert1_AlliedDisc.rar"
!define ALLIED_SIZE "524014608"
!define ALLIED_SHA256 "6ca7c7c18bce37fc4c60b0aeeb5aea104160ac5aa1e305bde4f5bfbfb32b41c6"
!define ALLIED_ISO "CD1_ALLIED_DISC.ISO"
!define SOVIET_RAR "RedAlert1_SovietDisc.rar"
!define SOVIET_SIZE "526433084"
!define SOVIET_SHA256 "a4e1b89a0489d516be835a4e85443a9fb84e3d530353f623e789054e4798f848"
!define SOVIET_ISO "CD2_SOVIET_DISC.ISO"
!define EA_EULA "EA Product EULA RedAlert1.rtf"
!endif
!ifdef SEVENZIP_DIR
!addplugindir /amd64-unicode "${RADL_DIR}"
!endif

!define MUI_ICON "${SOURCE_DIR}\CODE\REDALERT.ICO"
!define MUI_UNICON "${SOURCE_DIR}\CODE\REDALERT.ICO"
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_NODESC

!define MUI_WELCOMEPAGE_TITLE "Command && Conquer Red Alert"
!ifdef DATA_DIR
!define MUI_WELCOMEPAGE_TEXT "This installs Red Alert: a modern port of the game, built from the source code Electronic Arts released under the GPL, with the game data EA released as freeware in 2008.$\r$\n$\r$\nThe source code of this port is at:$\r$\n${SOURCE_URL}"
!else ifdef SEVENZIP_DIR
!define MUI_WELCOMEPAGE_TEXT "This installs Red Alert: a modern port of the game, built from the source code Electronic Arts released under the GPL.$\r$\n$\r$\nThe game data is EA's freeware release of the game's discs, from 2008: the setup can download it, or copy it from the discs.$\r$\n$\r$\nThe source code of this port is at:$\r$\n${SOURCE_URL}"
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
!ifndef DATA_DIR
Page custom Data_Page Data_Page_Leave		; Before the folder page, which shows the space needed.
!endif
!insertmacro MUI_PAGE_DIRECTORY
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
!ifdef SEVENZIP_DIR
	FileWrite $0 "$\r$\nThe setup program unpacks downloaded game data with 7-Zip (https://www.7-zip.org),$\r$\nunder the GNU LGPL with the unRAR restriction (see 7-Zip License.txt). Its source$\r$\ncode is at https://github.com/ip7z/7zip.$\r$\n"
	File "/oname=7-Zip License.txt" "${SEVENZIP_DIR}\License.txt"
!endif
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
	${If} $DataSource == "download"
!ifdef SEVENZIP_DIR
		Call Download_Data
!endif
	${ElseIf} $DataSource == "discs"
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

!ifdef SEVENZIP_DIR
;
;	Downloading the data. Each disc's .rar file is downloaded into the Download folder (and
;	checked), then 7-Zip unpacks its disc image, and the files the game needs from that.
;	A download that fails stays there, and running the setup again resumes it.
;
!macro Download_Disc RAR SIZE SHA256 ISO CD WHAT
	${If} ${FileExists} "$INSTDIR\${CD}\MAIN.MIX"
		DetailPrint "${WHAT} is installed already."
	${Else}
	retry_${CD}:
		DetailPrint "Downloading ${WHAT} (${RAR}, 500 MB)..."
		radl::download "$Mirror${RAR}" "$INSTDIR\Download\${RAR}" "${SIZE}" "${SHA256}" "${WHAT}"
		Pop $0
		${If} $0 != "OK"
			DetailPrint "Downloading ${WHAT} failed: $0"
			MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "Downloading ${WHAT} failed:$\r$\n$0$\r$\n$\r$\nTry again? If you cancel, the setup carries on without it; running the setup again later resumes the download." /SD IDCANCEL IDRETRY retry_${CD}
		${Else}
			DetailPrint "Unpacking ${WHAT}..."
			nsExec::ExecToLog '"$PLUGINSDIR\7-Zip\7z.exe" e -y -bso0 -bsp0 "-o$INSTDIR\Download" "$INSTDIR\Download\${RAR}" "${ISO}" "${EA_EULA}" -r'
			Pop $1
			CreateDirectory "$INSTDIR\${CD}"
			nsExec::ExecToLog '"$PLUGINSDIR\7-Zip\7z.exe" e -y -bso0 -bsp0 "-o$INSTDIR\${CD}" "$INSTDIR\Download\${ISO}" MAIN.MIX'
			Pop $1
			${IfNot} ${FileExists} "$INSTDIR\REDALERT.MIX"
				nsExec::ExecToLog '"$PLUGINSDIR\7-Zip\7z.exe" e -y -bso0 -bsp0 "-o$INSTDIR" "$INSTDIR\Download\${ISO}" "INSTALL\REDALERT.MIX"'
				Pop $1
			${EndIf}
			${If} ${FileExists} "$INSTDIR\Download\${EA_EULA}"
				Delete "$INSTDIR\EA License Agreement.rtf"
				Rename "$INSTDIR\Download\${EA_EULA}" "$INSTDIR\EA License Agreement.rtf"
			${EndIf}
			Delete "$INSTDIR\Download\${ISO}"
			${If} ${FileExists} "$INSTDIR\${CD}\MAIN.MIX"
				Delete "$INSTDIR\Download\${RAR}"
				DetailPrint "${WHAT} is installed."
			${Else}
				DetailPrint "Unpacking ${WHAT} failed (7-Zip: $1)."
				MessageBox MB_OK|MB_ICONEXCLAMATION "Unpacking ${WHAT} failed. Running the setup again tries again." /SD IDOK
			${EndIf}
		${EndIf}
	${EndIf}
!macroend

Function Download_Data
	CreateDirectory "$INSTDIR\Download"
	InitPluginsDir
	SetOutPath "$PLUGINSDIR\7-Zip"
	File "${SEVENZIP_DIR}\7z.exe"
	File "${SEVENZIP_DIR}\7z.dll"
	SetOutPath "$INSTDIR"
	!insertmacro Download_Disc "${ALLIED_RAR}" "${ALLIED_SIZE}" "${ALLIED_SHA256}" "${ALLIED_ISO}" "CD1" "the Allied disc"
	!insertmacro Download_Disc "${SOVIET_RAR}" "${SOVIET_SIZE}" "${SOVIET_SHA256}" "${SOVIET_ISO}" "CD2" "the Soviet disc"
	RMDir "$INSTDIR\Download"		; Unless a download failed.
FunctionEnd
!endif
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
;	Where the game data comes from.
;-------------------------------------------------------------------------------------
!ifndef DATA_DIR
; Removes a trailing backslash ("E:\" becomes "E:").
!macro Trim_Path VAR
	StrCpy $0 ${VAR} 1 -1
	${If} $0 == "\"
		StrCpy ${VAR} ${VAR} -1
	${EndIf}
!macroend

Function .onInit
	StrCpy $Mirror "${DATA_MIRROR}"
	${GetParameters} $R0
	ClearErrors
	${GetOptions} $R0 "/ALLIED=" $AlliedDisc
	ClearErrors
	${GetOptions} $R0 "/SOVIET=" $SovietDisc
	ClearErrors
	${GetOptions} $R0 "/DATA=" $DataSource
	!insertmacro Trim_Path $AlliedDisc
	!insertmacro Trim_Path $SovietDisc
	Call Read_Mirror_Option

	${If} $DataSource == ""
		${If} $AlliedDisc != ""
		${OrIf} $SovietDisc != ""
			StrCpy $DataSource "discs"
		${ElseIf} ${Silent}
			StrCpy $DataSource "none"
		${Else}
!ifdef SEVENZIP_DIR
			StrCpy $DataSource "download"
!else
			StrCpy $DataSource "discs"
!endif
		${EndIf}
	${EndIf}
!ifndef SEVENZIP_DIR
	${If} $DataSource == "download"
		StrCpy $DataSource "none"
	${EndIf}
!endif
	Call Set_Data_Size
FunctionEnd

; /MIRROR=URL: the rest of the parameter, up to a space. (GetOptions would stop at the URL's
; first slash.)
Function Read_Mirror_Option
	${GetParameters} $R0
	StrLen $R1 $R0
	StrCpy $R2 0
	find:
		IntCmp $R2 $R1 done 0 done
		StrCpy $R3 $R0 8 $R2
		StrCmp $R3 "/MIRROR=" found
		IntOp $R2 $R2 + 1
		Goto find
	found:
		IntOp $R2 $R2 + 8
		StrCpy $R4 ""
	copy:
		StrCpy $R3 $R0 1 $R2
		StrCmp $R3 "" copied
		StrCmp $R3 " " copied
		StrCmp $R3 '"' +2
		StrCpy $R4 "$R4$R3"
		IntOp $R2 $R2 + 1
		Goto copy
	copied:
		StrCmp $R4 "" done
		StrCpy $Mirror $R4
		StrCpy $R3 $Mirror 1 -1
		StrCmp $R3 "/" done
		StrCpy $Mirror "$Mirror/"
	done:
FunctionEnd

; The space the data needs: 1 GB, and while downloading another 1.5 GB for the archives.
Function Set_Data_Size
	${If} $DataSource == "download"
		SectionSetSize ${SecData} 2500000
	${ElseIf} $DataSource == "discs"
		SectionSetSize ${SecData} 960000
	${Else}
		SectionSetSize ${SecData} 0
	${EndIf}
FunctionEnd

Function Data_Page
	!insertmacro MUI_HEADER_TEXT "Game data" "Where should the game data come from?"
	nsDialogs::Create 1018
	Pop $0
	${NSD_CreateLabel} 0 0 100% 20u "The game data is on Red Alert's two discs: the Allied disc (CD 1) and the Soviet disc (CD 2), one for each campaign. EA released them as freeware in 2008."
	Pop $0
	StrCpy $DownloadRadio ""
!ifdef SEVENZIP_DIR
	${NSD_CreateRadioButton} 0 24u 100% 10u "Download EA's freeware release from the Internet Archive (1 GB)"
	Pop $DownloadRadio
	${NSD_OnClick} $DownloadRadio Data_Choice_Clicked
!endif
	${NSD_CreateRadioButton} 0 38u 100% 10u "Copy it from the discs (or disc images, or folders with their files):"
	Pop $DiscsRadio
	${NSD_OnClick} $DiscsRadio Data_Choice_Clicked

	${NSD_CreateLabel} 12u 53u 68u 10u "Allied disc (CD 1):"
	Pop $0
	${NSD_CreateDirRequest} 82u 51u 163u 13u "$AlliedDisc"
	Pop $AlliedField
	${NSD_CreateBrowseButton} 250u 51u 50u 13u "Browse..."
	Pop $AlliedBrowse
	nsDialogs::SetUserData $AlliedBrowse $AlliedField
	${NSD_OnClick} $AlliedBrowse Browse_For_Disc

	${NSD_CreateLabel} 12u 70u 68u 10u "Soviet disc (CD 2):"
	Pop $0
	${NSD_CreateDirRequest} 82u 68u 163u 13u "$SovietDisc"
	Pop $SovietField
	${NSD_CreateBrowseButton} 250u 68u 50u 13u "Browse..."
	Pop $SovietBrowse
	nsDialogs::SetUserData $SovietBrowse $SovietField
	${NSD_OnClick} $SovietBrowse Browse_For_Disc

	${NSD_CreateRadioButton} 0 88u 100% 10u "Don't install the game data now"
	Pop $NoneRadio
	${NSD_OnClick} $NoneRadio Data_Choice_Clicked

!ifdef SEVENZIP_DIR
	${NSD_CreateLabel} 0 104u 100% 20u "The downloaded files are checked against EA's originals. While installing, the download needs another 1.5 GB of free space."
!else
	${NSD_CreateLabel} 0 104u 100% 20u "Don't have the discs? EA's freeware release can be downloaded from the Internet Archive. Unpack the .rar files with 7-Zip, then double-click the .ISO files."
!endif
	Pop $0
	${NSD_CreateLink} 0 128u 100% 10u "About EA's freeware release (archive.org)"
	Pop $0
	SetCtlColors $0 0000CC transparent
	${NSD_OnClick} $0 Open_Data_Page

	${If} $DataSource == "download"
		${NSD_Check} $DownloadRadio
	${ElseIf} $DataSource == "none"
		${NSD_Check} $NoneRadio
	${Else}
		${NSD_Check} $DiscsRadio
	${EndIf}
	Call Enable_Disc_Fields
	nsDialogs::Show
FunctionEnd

Function Data_Choice_Clicked
	Pop $0
	${If} $DownloadRadio != ""
		${NSD_Uncheck} $DownloadRadio
	${EndIf}
	${NSD_Uncheck} $DiscsRadio
	${NSD_Uncheck} $NoneRadio
	${NSD_Check} $0
	Call Enable_Disc_Fields
FunctionEnd

Function Enable_Disc_Fields
	${NSD_GetState} $DiscsRadio $1
	${If} $1 == ${BST_CHECKED}
		StrCpy $1 1
	${Else}
		StrCpy $1 0
	${EndIf}
	EnableWindow $AlliedField $1
	EnableWindow $AlliedBrowse $1
	EnableWindow $SovietField $1
	EnableWindow $SovietBrowse $1
FunctionEnd

Function Open_Data_Page
	Pop $0
	ExecShell "open" "${DATA_URL}"
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

Function Data_Page_Leave
	${NSD_GetState} $DiscsRadio $0
	${NSD_GetState} $NoneRadio $1
	${If} $0 == ${BST_CHECKED}
		StrCpy $DataSource "discs"
		Call Check_Discs
	${ElseIf} $1 == ${BST_CHECKED}
		StrCpy $DataSource "none"
		MessageBox MB_YESNO|MB_ICONQUESTION "Install without the game data? The game can't run until it is in its folder (see README.md), but you can also run this setup again." IDYES +2
		Abort
	${Else}
		StrCpy $DataSource "download"
	${EndIf}
	Call Set_Data_Size
FunctionEnd

Function Check_Discs
	${NSD_GetText} $AlliedField $AlliedDisc
	${NSD_GetText} $SovietField $SovietDisc
	!insertmacro Trim_Path $AlliedDisc
	!insertmacro Trim_Path $SovietDisc

	${If} $AlliedDisc == ""
	${AndIf} $SovietDisc == ""
		MessageBox MB_OK|MB_ICONEXCLAMATION "Choose at least one of the discs."
		Abort
	${EndIf}
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
	${If} $1 == "no"
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
	Delete "$INSTDIR\7-Zip License.txt"
	RMDir /r "$INSTDIR\Download"				; Downloads that weren't finished.
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
