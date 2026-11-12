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
**	RAPORT.H -- game side glue for the modern (SDL2) port. See RAPORT.CPP.
*/
#ifndef RAPORT_H
#define RAPORT_H

void	Port_Select_Data_Directory(int argc, char * argv[]);

/*
**	CONQUER.CPP: RA_EXTRACT=NAME[,NAME...] copies files out of the MIX archives and exits.
*/
bool	Port_Extract_Files(void);

/*
**	CONQUER.CPP: discs are directories (CD1..CD4) in the data directory.
*/
extern int	Port_Current_CD;
bool	Port_CD_Present(int cd);
bool	Port_Get_CD_Directory(char * buffer);

/*
**	TACTION.CPP: trigger names of old format scenarios, waiting to be resolved.
*/
int	Port_Pending_Trigger_Name(char const * name);
char *	Port_Take_Pending_Trigger_Name(int raw);
void	Port_Clear_Pending_Trigger_Names(void);

#endif
