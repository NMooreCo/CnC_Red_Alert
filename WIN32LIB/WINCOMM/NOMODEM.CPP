/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
**	NOMODEM.CPP -- modem / null-modem (serial port) support for the modern port.
**
**	The original WINCOMM.CPP and MODEMREG.CPP drove COM ports through the Win32 serial API
**	and enumerated modems from the registry. Serial play is not supported by the port: no
**	modems are reported and opening a port always fails, so the game shows its normal
**	"unable to open port" message. The original files are kept for reference.
*/

#include "WINCOMM.H"
#include "MODEMREG.H"

WinModemClass * SerialPort = NULL;

WinModemClass::WinModemClass(void) :
	FramingErrors(0),
	IOErrors(0),
	BufferOverruns(0),
	InBufferOverflows(0),
	ParityErrors(0),
	OutBufferOverflows(0),
	InQueue(0),
	OutQueue(0)
{
	PortHandle = 0;
	SerialBuffer = NULL;
	EchoFunction = NULL;
	AbortFunction = NULL;
}

WinModemClass::~WinModemClass(void) {}
HANDLE WinModemClass::Serial_Port_Open(char * , int , int , int , int , int ) {return(INVALID_HANDLE_VALUE);}
void WinModemClass::Serial_Port_Close(void) {}
int WinModemClass::Read_From_Serial_Port(unsigned char * , int ) {return(0);}
void WinModemClass::Write_To_Serial_Port(unsigned char * , int ) {}
void WinModemClass::Wait_For_Serial_Write(void) {}
void WinModemClass::Set_Modem_Dial_Type(WinCommDialMethodType ) {}
unsigned WinModemClass::Get_Modem_Status(void) {return(0);}
void WinModemClass::Set_Serial_DTR(BOOL ) {}
int WinModemClass::Get_Modem_Result(int , char * buffer, int buffer_len) {if (buffer && buffer_len) buffer[0] = 0; return(0);}
void WinModemClass::Dial_Modem(char * ) {}
int WinModemClass::Send_Command_To_Modem(char * , char , char * buffer, int buflen, int , int ) {if (buffer && buflen) buffer[0] = 0; return(MODEM_CMD_TIMEOUT);}
void WinModemClass::Set_Echo_Function(void(*func)(char c)) {EchoFunction = func;}
void WinModemClass::Set_Abort_Function(int (*func)(void)) {AbortFunction = func;}
HANDLE WinModemClass::Get_Port_Handle(void) {return(INVALID_HANDLE_VALUE);}

ModemRegistryEntryClass::ModemRegistryEntryClass(int ) :
	ModemName(NULL),
	ModemDeviceName(NULL),
	ErrorCorrectionEnable(NULL),
	ErrorCorrectionDisable(NULL),
	CompressionEnable(NULL),
	CompressionDisable(NULL),
	HardwareFlowControl(NULL),
	NoFlowControl(NULL)
{
}

ModemRegistryEntryClass::~ModemRegistryEntryClass(void) {}
