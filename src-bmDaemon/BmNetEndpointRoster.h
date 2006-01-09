/*
	BmNetEndpointRoster.cpp

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

#ifndef _BmNetEndpointRoster_h
#define _BmNetEndpointRoster_h

#include <Message.h>

#include "BmDaemon.h"

class BmNetEndpoint;

class IMPEXPBMDAEMON BmNetEndpointRoster {
	typedef BmNetEndpoint* (*BmInstantiateNetEndpointFunc)();
	typedef status_t (*BmGetEncryptionInfoFunc)(BMessage*);
public:
	BmNetEndpointRoster();
	~BmNetEndpointRoster();

	BmNetEndpoint* CreateEndpoint();
	//
 	bool SupportsEncryption();
 	bool SupportsEncryptionType(const char* encType);
	status_t GetEncryptionInfo(BMessage* encryptionInfo);
private:
	void _Initialize();
	void _Cleanup();

	bool mNeedInit;
	image_id mAddonImage;
	BmString mAddonName;
	BmInstantiateNetEndpointFunc mAddonInstantiateFunc;
	BmGetEncryptionInfoFunc mAddonGetEncryptionInfoFunc;
	BMessage mEncryptionInfo;
};

extern IMPEXPBMDAEMON BmNetEndpointRoster* TheNetEndpointRoster;

#endif
