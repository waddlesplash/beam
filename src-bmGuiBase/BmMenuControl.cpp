/*
	BmMenuControl.cpp
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


#include <MenuItem.h>

#include <HGroup.h>

#include "Colors.h"

#include "split.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmMenuControl.h"
#include "BmMenuControllerBase.h"


/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::BmMenuControl( const char* label, BMenu* menu, float weight, 
										float maxWidth, const char* fitText)
	:	inherited( BRect(0,0,400,20), NULL, label, menu, true, B_FOLLOW_NONE)
	,	mMenu( static_cast<BMenu*>( ChildAt( 0)))
{
	ResizeToPreferred();
	BRect b = Bounds();
	float labelWidth = StringWidth( label);
	if (BeamOnDano)
		SetDivider( 13 + (label ? labelWidth+19 : 0));
	else
		SetDivider( (label ? labelWidth+19 : 0));
	if (fitText) {
		float fixedWidth = StringWidth( fitText)+Divider()+27;
		ct_mpm = minimax( fixedWidth, b.Height()+4, 
								fixedWidth, b.Height()+4);
	} else {
		ct_mpm = minimax( 
			StringWidth("1234567890")+Divider()+27, 
			b.Height()+4, maxWidth, b.Height()+4, weight
		);
	}
	ResizeTo( ct_mpm.mini.x, ct_mpm.mini.y);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::~BmMenuControl() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::MarkItem( const char* label) {
	BmMenuControllerBase* menuContr 
		= dynamic_cast< BmMenuControllerBase*>( Menu());
	MenuItem()->SetLabel( label);
	if (menuContr) {
		menuContr->MarkItem( label);
		BmMenuControllerBase::MarkItemInMenu( menuContr, label);
	} else {
		BMenuItem* item = Menu()->FindItem( label);
		if (item)
			item->SetMarked( true);
		else
			ClearMark();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::ClearMark() {
	MenuItem()->SetLabel("");
	BmMenuControllerBase::ClearMarkInMenu( Menu());
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::SetEnabled( bool enabled) {
	inherited::SetEnabled( enabled);
	if (enabled)
		SetFlags( Flags() | B_NAVIGABLE);
	else
		SetFlags( Flags() & (0xFFFFFFFF^B_NAVIGABLE));
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
minimax BmMenuControl::layoutprefs() {
	return mpm = ct_mpm;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::SetDivider( float divider) {
	if (BeamOnDano)
		inherited::SetDivider( divider-13);
	else
		inherited::SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMenuControl::Divider() const {
	return inherited::Divider();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmMenuControl::layout(BRect frame) {
	if (frame == Frame())
		return frame;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	float occupiedSpace = BeamOnDano
									? Divider()+1+13
									: Divider()-12;
	if (occupiedSpace < 3)
		occupiedSpace = 3;					// leave room for focus-rectangle
	mMenu->MoveTo( occupiedSpace, mMenu->Frame().top);
	mMenu->ResizeTo( frame.Width()-occupiedSpace-6, mMenu->Frame().Height());
	mMenu->Invalidate();
	return frame;
}
