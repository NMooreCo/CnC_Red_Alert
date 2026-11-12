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

/***********************************************************************************************
 *                 Project Name : Command & Conquer                                            *
 *                    File Name : PHONE.H                                                      *
 *                                                                                             *
 *   This file was not part of the source release. It has been reconstructed for the modern  *
 *   port from the way the modem phone book is used in SESSION.CPP and NULLDLG.CPP.           *
 *---------------------------------------------------------------------------------------------*/

#ifndef PHONE_H
#define PHONE_H

/*
**	One entry in the modem phone book.
*/
class PhoneEntryClass
{
	public:
		enum PhoneEntryEnum {
			PHONE_MAX_NAME = 21,		// Max length of a name, including the terminating NULL.
			PHONE_MAX_NUM = 21		// Max length of a phone number, including the NULL.
		};

		PhoneEntryClass(void) {
			Name[0] = 0;
			Number[0] = 0;
			memset(&Settings, 0, sizeof(Settings));
		};
		~PhoneEntryClass() {};

		int operator == (PhoneEntryClass const & obj) const {
			return(strcmp(Name, obj.Name) == 0 && strcmp(Number, obj.Number) == 0);
		};

		char Name[PHONE_MAX_NAME];
		char Number[PHONE_MAX_NUM];
		SerialSettingsType Settings;
};

#endif
