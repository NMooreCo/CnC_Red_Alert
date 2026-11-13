#
#	Command & Conquer Red Alert(tm)
#	Copyright 2025 Electronic Arts Inc.
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#
#	Modern build of Red Alert (Win95 code path) for 64-bit Linux/macOS/Windows with SDL2.
#
#	The original game was built with Watcom C/C++ 10.6 and Borland TASM (see CODE/MAKEFILE).
#	This build replaces the assembly language with C++ and DirectX/Win32 with SDL2.
#
cmake_minimum_required(VERSION 3.16)
project(RedAlert LANGUAGES C CXX)
if(WIN32)
	enable_language(RC)		# The program's icon and version information (CODE/RAPORT.RC).
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(RA_LANGUAGE_GERMAN "Build the German language version" OFF)
option(RA_LANGUAGE_FRENCH "Build the French language version" OFF)
option(RA_WARNINGS "Enable compiler warnings for the legacy code" OFF)
option(RA_SANITIZE "Build with AddressSanitizer/UBSan (debugging)" OFF)
option(RA_ENHANCEMENTS "Build in the optional gameplay enhancements (veterancy...); each is off until the player turns it on" ON)
option(RA_DEVELOPER "Build in Westwood's scenario editor and developer keys (-editor, -cheats on the command line)" ON)

find_package(SDL2 REQUIRED)

#---------------------------------------------------------------------------
# Common compile settings shared by all targets.
#---------------------------------------------------------------------------
add_library(ra_common INTERFACE)
target_include_directories(ra_common INTERFACE
	${CMAKE_SOURCE_DIR}/compat/include
)
if(NOT WIN32)
	target_include_directories(ra_common INTERFACE ${CMAKE_SOURCE_DIR}/compat/posix)
endif()

if(MSVC)
	target_compile_options(ra_common INTERFACE
		/FI${CMAKE_SOURCE_DIR}/compat/portable.h
		/permissive /J- /W0 /MP
	)
	target_compile_definitions(ra_common INTERFACE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
	# (Only for C and C++: on Windows the resource compiler gets the targets' options too.)
	target_compile_options(ra_common INTERFACE
		$<$<COMPILE_LANGUAGE:C,CXX>:-include ${CMAKE_SOURCE_DIR}/compat/portable.h>
		$<$<COMPILE_LANGUAGE:C,CXX>:-fsigned-char>				# Watcom /j : char is signed.
		$<$<COMPILE_LANGUAGE:C,CXX>:-fwrapv>						# Legacy code relies on 2's complement wrap around.
		$<$<COMPILE_LANGUAGE:C,CXX>:-fno-strict-aliasing>		# Legacy code type puns freely.
	)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(ra_common INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# Byte tables are initialised with values above 127 in (signed) char arrays.
		target_compile_options(ra_common INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wno-c++11-narrowing>)
	endif()
	if(NOT RA_WARNINGS)
		target_compile_options(ra_common INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-w>)
	endif()
	if(MINGW)
		# Lay bit-fields out as GCC does elsewhere, not as Microsoft's compiler does (MinGW's
		# default), so that network packets and saved games are the same as on Linux.
		target_compile_options(ra_common INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-mno-ms-bitfields>)
	endif()
	if(RA_SANITIZE)
		# -fsanitize-recover=address: with ASAN_OPTIONS=halt_on_error=0 a run reports every
		# error and keeps going (by default it still stops at the first).
		target_compile_options(ra_common INTERFACE
			$<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize=address,undefined>
			$<$<COMPILE_LANGUAGE:C,CXX>:-fsanitize-recover=address>
			$<$<COMPILE_LANGUAGE:C,CXX>:-fno-omit-frame-pointer>
		)
		target_link_options(ra_common INTERFACE -fsanitize=address,undefined -fsanitize-recover=address)
	endif()
endif()

#---------------------------------------------------------------------------
# Settings for the original game & library code.
#---------------------------------------------------------------------------
add_library(ra_game_defs INTERFACE)
target_compile_definitions(ra_game_defs INTERFACE
	WIN32=1					# Selects the Win95 code paths of the game (hi-res, etc.)
	WINSOCK_IPX=1			# Network games use IPX/UDP through (emulated) Winsock, as in the original
	RA_PORT=1				# Marks code added/changed for the modern port
	RA_PACK_STRUCTS=1		# Byte packed structures, as Watcom /zp1 (see compat/portable.h)
)
if(NOT MSVC)
	# Watcom sized each enum to the smallest integer type that holds its values (it had no -ei),
	# and data files and structures depend on that (e.g. 2 byte map template numbers).
	target_compile_options(ra_game_defs INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-fshort-enums>)
	# A few checks of the form "this != NULL" are relied on (GenericNode::Is_Valid).
	target_compile_options(ra_game_defs INTERFACE $<$<COMPILE_LANGUAGE:C,CXX>:-fno-delete-null-pointer-checks>)
	# The game's pooled classes have an operator new that returns NULL when the pool is full,
	# and the code checks for that. Pre-standard compilers skipped the constructor then;
	# modern ones assume operator new never returns NULL unless told otherwise.
	include(CheckCXXCompilerFlag)
	check_cxx_compiler_flag(-fcheck-new RA_HAVE_CHECK_NEW)
	if(RA_HAVE_CHECK_NEW)
		target_compile_options(ra_game_defs INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-fcheck-new>)
	endif()
endif()
if(RA_LANGUAGE_GERMAN)
	target_compile_definitions(ra_game_defs INTERFACE GERMAN=1)
elseif(RA_LANGUAGE_FRENCH)
	target_compile_definitions(ra_game_defs INTERFACE FRENCH=1)
else()
	target_compile_definitions(ra_game_defs INTERFACE ENGLISH=1)
endif()
if(RA_ENHANCEMENTS)
	target_compile_definitions(ra_game_defs INTERFACE RA_ENHANCE=1)	# Marks the optional enhancements
endif()
if(RA_DEVELOPER)
	# What Westwood's internal builds had (DEFINES.H, INTERNAL_VERSION): the scenario editor and
	# the developer keys. Both stay off unless switched on (-editor, -cheats).
	target_compile_definitions(ra_game_defs INTERFACE SCENARIO_EDITOR=1 CHEAT_KEYS=1)
endif()
target_link_libraries(ra_game_defs INTERFACE ra_common)

#---------------------------------------------------------------------------
# Platform layer: Win32 API emulation (compat/) and SDL2 video/audio/input
# (WIN32LIB/SDL). Built without the game's WIN32 macro so SDL detects the
# real host platform.
#---------------------------------------------------------------------------
add_library(ra_platform STATIC
	compat/compat.cpp
	compat/win32.cpp
	compat/winsock.cpp
	WIN32LIB/SDL/SDLVIDEO.CPP
	WIN32LIB/SDL/SDLSOUND.CPP
)
target_include_directories(ra_platform PRIVATE ${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE ${CMAKE_SOURCE_DIR}/compat)
target_link_libraries(ra_platform PUBLIC ra_common SDL2::SDL2)
if(WIN32)
	target_link_libraries(ra_platform PUBLIC ws2_32)		# Network games use the real Winsock.
endif()

#---------------------------------------------------------------------------
# WIN32LIB: Westwood's 32 bit library (graphics buffers, shapes, fonts, files...).
#---------------------------------------------------------------------------
set(WIN32LIB_SOURCES
	WIN32LIB/AUDIO/AUDCODEC.CPP
	WIN32LIB/AUDIO/SOUNDINT.CPP
	WIN32LIB/AUDIO/SOUNDIO.CPP
	WIN32LIB/AUDIO/SOUNDLCK.CPP
	WIN32LIB/WINCOMM/NOMODEM.CPP
	WIN32LIB/DIPTHONG/DIPTHONG.CPP
	WIN32LIB/DIPTHONG/_DIPTABL.CPP
	WIN32LIB/DRAWBUFF/BLITTERS.CPP
	WIN32LIB/DRAWBUFF/BUFFER.CPP
	WIN32LIB/DRAWBUFF/BUFFGLBL.CPP
	WIN32LIB/DRAWBUFF/DRAWRECT.CPP
	WIN32LIB/DRAWBUFF/GBUFFER.CPP
	WIN32LIB/DRAWBUFF/ICONCACH.CPP
	WIN32LIB/DRAWBUFF/REGIONSZ.CPP
	WIN32LIB/FONT/FONT.CPP
	WIN32LIB/FONT/LOADFONT.CPP
	WIN32LIB/FONT/SET_FONT.CPP
	WIN32LIB/IFF/IFF.CPP
	WIN32LIB/IFF/LOAD.CPP
	WIN32LIB/IFF/LOADPICT.CPP
	WIN32LIB/IFF/WRITELBM.CPP
	WIN32LIB/KEYBOARD/MOUSE.CPP
	WIN32LIB/MEM/ALLOC.CPP
	WIN32LIB/MEM/MEM.CPP
	WIN32LIB/MEM/NEWDEL.CPP
	WIN32LIB/MISC/DDRAW.CPP
	WIN32LIB/MISC/DELAY.CPP
	WIN32LIB/MISC/EXIT.CPP
	WIN32LIB/MISC/FINDARGV.CPP
	WIN32LIB/MISC/IRANDOM.CPP
	WIN32LIB/MISC/LIB.CPP
	WIN32LIB/MISC/MISCASM.CPP
	WIN32LIB/MISC/VERSION.CPP
	WIN32LIB/PALETTE/LOADPAL.CPP
	WIN32LIB/PALETTE/MORPHPAL.CPP
	WIN32LIB/PALETTE/PALETTE.CPP
	WIN32LIB/PLAYCD/GETCD.CPP
	WIN32LIB/SHAPE/GETSHAPE.CPP
	WIN32LIB/TILE/ICONSET.CPP
	WIN32LIB/TIMER/TIMER.CPP
	WIN32LIB/TIMER/TIMERDWN.CPP
	WIN32LIB/TIMER/TIMERINI.CPP
	WIN32LIB/WSA/WSA.CPP
	WIN32LIB/WW_WIN/WINDOWS.CPP
	WIN32LIB/WW_WIN/WINHIDE.CPP
)
add_library(win32lib STATIC ${WIN32LIB_SOURCES})
target_include_directories(win32lib PRIVATE ${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE)
target_link_libraries(win32lib PUBLIC ra_game_defs ra_platform)

#---------------------------------------------------------------------------
# WINVQ: the VQA movie player.
#---------------------------------------------------------------------------
set(WINVQ_SOURCES
	WINVQ/VQA32/AUDIO.CPP
	WINVQ/VQA32/CAPTION.CPP
	WINVQ/VQA32/CONFIG.CPP
	WINVQ/VQA32/DRAWER.CPP
	WINVQ/VQA32/DSTREAM.CPP
	WINVQ/VQA32/LOADER.CPP
	WINVQ/VQA32/MONODISP.CPP
	WINVQ/VQM32/PROFILE.CPP
	WINVQ/VQA32/TASK.CPP
	WINVQ/VQA32/UNVQBUFF.CPP
	WINVQ/VQA32/VERTAG.CPP
	WINVQ/VQM32/AUDCODEC.CPP
	WINVQ/VQM32/AUDZAP.CPP
	WINVQ/VQM32/CAPTOKEN.CPP
	WINVQ/VQM32/CHRWIDTH.CPP
	WINVQ/VQM32/FONT.CPP
	WINVQ/VQM32/MEM.CPP
)
add_library(winvq STATIC ${WINVQ_SOURCES})
target_include_directories(winvq PRIVATE
	${CMAKE_SOURCE_DIR}/WINVQ/INCLUDE/VQA32
	${CMAKE_SOURCE_DIR}/WINVQ/INCLUDE/VQM32
	${CMAKE_SOURCE_DIR}/WINVQ/INCLUDE
	${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE
)
target_link_libraries(winvq PUBLIC ra_game_defs ra_platform)

#---------------------------------------------------------------------------
# The game itself.
#---------------------------------------------------------------------------
set(GAME_SOURCES
	CODE/2KEYFBUF.CPP CODE/2KEYFRAM.CPP CODE/AADATA.CPP CODE/ABSTRACT.CPP CODE/ADATA.CPP CODE/AIRCRAFT.CPP
	CODE/ANIM.CPP CODE/AUDIO.CPP CODE/B64PIPE.CPP CODE/B64STRAW.CPP CODE/BAR.CPP CODE/BASE.CPP
	CODE/BASE64.CPP CODE/BBDATA.CPP CODE/BDATA.CPP CODE/BENCH.CPP CODE/BFIOFILE.CPP
	CODE/BLOWFISH.CPP CODE/BLOWPIPE.CPP CODE/BLWSTRAW.CPP CODE/BUFF.CPP CODE/BUILDING.CPP
	CODE/BULLET.CPP CODE/CARGO.CPP CODE/CARRY.CPP CODE/CCFILE.CPP CODE/CCINI.CPP CODE/CCPTR.CPP
	CODE/CDATA.CPP CODE/CDFILE.CPP CODE/CELL.CPP CODE/CHECKBOX.CPP CODE/CHEKLIST.CPP
	CODE/COLRLIST.CPP CODE/COMBAT.CPP CODE/COMBUF.CPP CODE/CONNECT.CPP CODE/CONQUER.CPP
	CODE/CONST.CPP CODE/CONTROL.CPP CODE/COORD.CPP CODE/CRATE.CPP CODE/CRC.CPP CODE/CRCPIPE.CPP
	CODE/CRCSTRAW.CPP CODE/CREDITS.CPP CODE/CREW.CPP CODE/DEBUG.CPP CODE/DIAL8.CPP CODE/DIALOG.CPP
	CODE/DISPLAY.CPP CODE/DOOR.CPP CODE/DRIVE.CPP CODE/DROP.CPP CODE/DYNAVEC.CPP CODE/EDIT.CPP
	CODE/EGOS.CPP CODE/ENDING.CPP CODE/EVENT.CPP CODE/EXPAND.CPP CODE/FACE.CPP CODE/FACING.CPP
	CODE/FACTORY.CPP CODE/FINDPATH.CPP CODE/FIXED.CPP CODE/FLASHER.CPP CODE/FLY.CPP CODE/FOOT.CPP
	CODE/FUSE.CPP CODE/GADGET.CPP CODE/GAMEDLG.CPP CODE/GAUGE.CPP CODE/GETCPU.CPP CODE/GLOBALS.CPP
	CODE/GOPTIONS.CPP CODE/GSCREEN.CPP CODE/HDATA.CPP CODE/HEAP.CPP CODE/HELP.CPP CODE/HOUSE.CPP
	CODE/HSV.CPP CODE/IDATA.CPP CODE/INFANTRY.CPP CODE/INI.CPP CODE/INIT.CPP CODE/INT.CPP
	CODE/INTERPAL.CPP CODE/INTRO.CPP CODE/IOMAP.CPP CODE/IOOBJ.CPP CODE/IPXADDR.CPP
	CODE/IPXCONN.CPP CODE/IPXGCONN.CPP CODE/IPXMGR.CPP CODE/JSHELL.CPP CODE/KEY.CPP CODE/LAYER.CPP
	CODE/LCW.CPP CODE/LCWPIPE.CPP CODE/LCWSTRAW.CPP CODE/LINK.CPP CODE/LIST.CPP CODE/LOADDLG.CPP
	CODE/LOGIC.CPP CODE/LZO1X_C.CPP CODE/LZO1X_D.CPP CODE/LZOPIPE.CPP CODE/LZOSTRAW.CPP
	CODE/LZW.CPP CODE/LZWPIPE.CPP CODE/LZWSTRAW.CPP CODE/MAP.CPP CODE/MAPEDDLG.CPP
	CODE/MAPEDIT.CPP CODE/MAPEDPLC.CPP CODE/MAPEDTM.CPP CODE/MAPSEL.CPP CODE/MENUS.CPP
	CODE/MISSION.CPP CODE/MIXFILE.CPP CODE/MONOC.CPP CODE/MOUSE.CPP CODE/MP.CPP CODE/MPLAYER.CPP
	CODE/MPU.CPP CODE/MSGBOX.CPP CODE/MSGLIST.CPP CODE/NETDLG.CPP CODE/NULLCONN.CPP
	CODE/NULLDLG.CPP CODE/NULLMGR.CPP CODE/OBJECT.CPP CODE/ODATA.CPP CODE/OPTIONS.CPP
	CODE/OVERLAY.CPP CODE/PACKET.CPP CODE/PALETTE.CPP CODE/PIPE.CPP CODE/PK.CPP CODE/PKPIPE.CPP
	CODE/PKSTRAW.CPP CODE/POWER.CPP CODE/PROFILE.CPP CODE/QUEUE.CPP CODE/RADAR.CPP CODE/RADIO.CPP
	CODE/RAMFILE.CPP CODE/RANDOM.CPP CODE/RAWFILE.CPP CODE/READLINE.CPP CODE/RECT.CPP
	CODE/REINF.CPP CODE/RGB.CPP CODE/RNDSTRAW.CPP CODE/ROTBMP.CPP CODE/RULES.CPP
	CODE/SAVELOAD.CPP CODE/SCENARIO.CPP CODE/SCORE.CPP CODE/SCROLL.CPP CODE/SDATA.CPP
	CODE/SENDFILE.CPP CODE/SESSION.CPP CODE/SHA.CPP CODE/SHAPEBTN.CPP CODE/SHAPIPE.CPP
	CODE/SHASTRAW.CPP CODE/SIDEBAR.CPP CODE/SLIDER.CPP CODE/SMUDGE.CPP CODE/SOUNDDLG.CPP
	CODE/SPECIAL.CPP CODE/SPRITE.CPP CODE/STARTUP.CPP CODE/STATBTN.CPP
	CODE/STRAW.CPP CODE/SUPER.CPP CODE/TAB.CPP CODE/TACTION.CPP CODE/TARGET.CPP CODE/TDATA.CPP
	CODE/TEAM.CPP CODE/TEAMTYPE.CPP CODE/TECHNO.CPP CODE/TEMPLATE.CPP CODE/TERRAIN.CPP
	CODE/TEVENT.CPP CODE/TEXTBTN.CPP CODE/THEME.CPP CODE/TOGGLE.CPP CODE/TRACKER.CPP
	CODE/TRIGGER.CPP CODE/TRIGTYPE.CPP CODE/TXTLABEL.CPP CODE/UDATA.CPP CODE/UDPADDR.CPP
	CODE/UNIT.CPP CODE/UTRACKER.CPP CODE/VDATA.CPP CODE/VECTOR.CPP CODE/VERSION.CPP
	CODE/VESSEL.CPP CODE/VISUDLG.CPP CODE/VORTEX.CPP CODE/WARHEAD.CPP CODE/WEAPON.CPP
	CODE/WINSTUB.CPP CODE/WRITEPCX.CPP CODE/WSPIPX.CPP CODE/WSPROTO.CPP CODE/WSPUDP.CPP
	CODE/XPIPE.CPP CODE/XSTRAW.CPP CODE/_WSPROTO.CPP CODE/FIELD.CPP
	CODE/RAPORT.CPP CODE/WINASM.CPP CODE/WOLSTRNG.CPP
	CODE/ENHANCE.CPP CODE/SMARTAI.CPP CODE/SURVIVAL.CPP
)
# Not built (services that no longer exist):
#   TCPIP.CPP INTERNET.CPP   - WChat/Planet Westwood internet games (see CODE/PORTSTUB.CPP)
#   STATS.CPP TOOLTIP.CPP    - Westwood Online statistics / lobby UI (WOLAPI_INTEGRATION)
#   WOL_*.CPP WOLAPIOB.CPP RAWOLAPI.CPP COMINIT.CPP ... - Westwood Online (COM based)
#   IPX95.CPP                - IPX thunk DLL for Win95 (WINSOCK_IPX is used instead)
#   MCI*.CPP MPGSET.CPP      - DVD version MPEG movie support (DirectX Media)
#   CCDDE.CPP DDE.CPP        - DDE link to WChat
add_executable(redalert ${GAME_SOURCES})
# Order matters: several CODE headers share names with library headers.
target_include_directories(redalert PRIVATE
	${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE
	${CMAKE_SOURCE_DIR}/WINVQ/INCLUDE
	${CMAKE_SOURCE_DIR}/CODE
)
if(WIN32)
	# A Windows (not console) program. Its own main() is the entry point (MinGW's start-up
	# code calls it), so SDL2main isn't used.
	set_target_properties(redalert PROPERTIES WIN32_EXECUTABLE ON)
	target_sources(redalert PRIVATE CODE/RAPORT.RC)
	target_link_libraries(redalert PRIVATE win32lib winvq ra_platform ra_game_defs)
	if(MINGW)
		# Link the C and C++ runtime in, so that the program only needs SDL2.dll next to it.
		target_link_options(redalert PRIVATE -static)
	endif()
else()
	target_link_libraries(redalert PRIVATE win32lib winvq ra_platform ra_game_defs SDL2::SDL2main)
endif()
