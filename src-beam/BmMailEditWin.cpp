/*
	BmMailEditWin.cpp
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


#include <File.h>
#include <FilePanel.h>
#include <InterfaceKit.h>
#include <Message.h>
#include "BmString.h"

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "PrefilledBitmap.h"
#include "TextEntryAlert.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMailEditWin.h"
#include "BmMenuControl.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"
#include "BmPeople.h"


/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
#define BM_TO_CC_BCC_CLEAR 	'bMYa'
#define BM_TO_CC_BCC_ADDED	 	'bMYb'
#define BM_CHARSET_SELECTED	'bMYc'
#define BM_FROM_ADDED 			'bMYd'
#define BM_FROM_SET	 			'bMYe'
#define BM_SMTP_SELECTED		'bMYf'
#define BM_EDIT_HEADER_DONE	'bMYg'
#define BM_SHOWDETAILS1			'bMYh'
#define BM_SHOWDETAILS2			'bMYi'
#define BM_SHOWDETAILS3			'bMYj'
#define BM_SIGNATURE_SELECTED	'bMYk'


/********************************************************************************\
	BmPeopleControl
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmPeopleControl::BmPeopleControl( const char* label)
	:	inherited( label, true)
	,	inheritedController( label)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmPeopleControl::~BmPeopleControl() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleControl::AttachedToWindow( void) {
	inherited::AttachedToWindow();
	// connect to the people-list:
	StartJob( ThePeopleList.Get(), false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleControl::DetachedFromWindow( void) {
	// disconnect from people-list:
	DetachModel();
	inherited::DetachedFromWindow();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleControl::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOB_DONE:
			case BM_LISTMODEL_ADD:
			case BM_LISTMODEL_UPDATE:
			case BM_LISTMODEL_REMOVE: {
				// handle job-related messages (related to our menu):
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (item)
					item->RemoveRef();		// the msg is no longer referencing the item
				JobIsDone( true);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( ModelNameNC() << ": " << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmPeopleControl::JobIsDone( bool completed) {
	if (completed) {
		BmAutolockCheckGlobal lock( DataModel()->ModelLocker());
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( BmString() << ControllerName() << ":JobIsDone(): Unable to lock model");
		BMenuItem* old;
		while( (old = Menu()->RemoveItem( (int32)0))!=NULL)
			delete old;

		// add all adresses to menu and a menu-entry for clearing the field:
		BMessage templateMsg( BM_TO_CC_BCC_ADDED);
		templateMsg.AddPointer( BmMailEditWin::MSG_CONTROL, this);
		ThePeopleList->AddPeopleToMenu( Menu(), templateMsg, BmMailEditWin::MSG_ADDRESS);
		BMessage* clearMsg = new BMessage( BM_TO_CC_BCC_CLEAR);
		clearMsg->AddPointer( BmMailEditWin::MSG_CONTROL, this);
		Menu()->AddSeparatorItem();
		Menu()->AddItem( new BMenuItem( "<Clear Field>", clearMsg));
	}
}



/********************************************************************************\
	BmMailEditWin
\********************************************************************************/

float BmMailEditWin::nNextXPos = 300;
float BmMailEditWin::nNextYPos = 100;
BmMailEditWin::BmEditWinMap BmMailEditWin::nEditWinMap;

const char* const BmMailEditWin::MSG_CONTROL = 	"ctrl";
const char* const BmMailEditWin::MSG_ADDRESS = 	"addr";

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMailRef* mailRef) {
	BmEditWinMap::iterator pos = nEditWinMap.find( mailRef->EntryRef());
	if (pos != nEditWinMap.end())
		return pos->second;
	BmMailEditWin* win = new BmMailEditWin( mailRef, NULL);
	win->ReadStateInfo();
	nEditWinMap[mailRef->EntryRef()] = win;
	return win;
}

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMail* mail) {
	BmMailRef* mailRef = mail->MailRef();
	if (mailRef) {
		BmEditWinMap::iterator pos = nEditWinMap.find( mailRef->EntryRef());
		if (pos != nEditWinMap.end())
			return pos->second;
	}
	BmMailEditWin* win = new BmMailEditWin( NULL, mail);
	win->ReadStateInfo();
	if (mailRef)
		nEditWinMap[mailRef->EntryRef()] = win;
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::BmMailEditWin( BmMailRef* mailRef, BmMail* mail)
	:	inherited( "MailEditWin", BRect(50,50,800,600), "Edit Mail", 
					  ThePrefs->GetBool( "UseDocumentResizer", false) 
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails1( false)
	,	mShowDetails2( false)
	,	mShowDetails3( false)
	,	mModified( false)
	,	mHasNeverBeenSaved( mail ? mail->MailRef() == NULL : false)
	,	mAttachPanel( NULL)
{
	CreateGUI();
	mMailView->AddFilter( new BmShiftTabMsgFilter( mSubjectControl, B_KEY_DOWN));
	mToControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
	mCcControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
	mBccControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
	if (mail)
		EditMail( mail);
	else
		EditMail( mailRef);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::~BmMailEditWin() {
	delete mAttachPanel;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		BmMailRef* mailRef = mail->MailRef();
		if (mailRef)
			nEditWinMap.erase(mailRef->EntryRef());
	}
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailEditWin::BmShiftTabMsgFilter::Filter( BMessage* msg, BHandler**) {
	if (msg->what == B_KEY_DOWN) {
		BmString bytes = msg->FindString( "bytes");
		int32 modifiers = msg->FindInt32( "modifiers");
		if (bytes.Length() && bytes[0]==B_TAB && modifiers & B_SHIFT_KEY) {
			mShiftTabToControl->MakeFocus( true);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailEditWin::BmPeopleDropMsgFilter::Filter( BMessage* msg, 
																				BHandler** handler) {
	filter_result res = B_DISPATCH_MESSAGE;
	BView* cntrl = handler ? dynamic_cast< BView*>( *handler) : NULL;
	if (msg && msg->what == B_SIMPLE_DATA && cntrl) {
		BmMailEditWin* win = dynamic_cast< BmMailEditWin*>( cntrl->Window());
		if (!win)
			return res;
		entry_ref eref;
		for( int32 i=0; msg->FindRef( "refs", i, &eref) == B_OK; ++i) {
			if (CheckMimeType( &eref, "application/x-person")) {
				BNode personNode( &eref);
				if (personNode.InitCheck() != B_OK)
					continue;
				BmString addr;
				BmReadStringAttr( &personNode, "META:email", addr);
				win->AddAddressToTextControl( dynamic_cast< BmTextControl*>( cntrl), addr);
				res = B_SKIP_MESSAGE;
			}
		}
	}
	return res;
}

/*------------------------------------------------------------------------------*\
	CreateGUI()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::CreateGUI() {
	// Get maximum button size
	float width=0, height=0;
	BmToolbarButton::CalcMaxSize(width, height, "Send",		TheResources->IconByName("Button_Send"));
	BmToolbarButton::CalcMaxSize(width, height, "Save",		TheResources->IconByName("Button_Save"));
	BmToolbarButton::CalcMaxSize(width, height, "New",			TheResources->IconByName("Button_New"));
	BmToolbarButton::CalcMaxSize(width, height, "Attach",		TheResources->IconByName("Attachment"));
	BmToolbarButton::CalcMaxSize(width, height, "People",		TheResources->IconByName("Person"));
	BmToolbarButton::CalcMaxSize(width, height, "Print",		TheResources->IconByName("Button_Print"));
	mOuterGroup = 
		new VGroup(
			minimax( 200, 300, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mSendButton = new BmToolbarButton( "Send", 
																  TheResources->IconByName("Button_Send"), 
																  width, height,
																  new BMessage(BMM_SEND_NOW), this, 
																  "Send mail now", true),
					mSaveButton = new BmToolbarButton( "Save", 
																	TheResources->IconByName("Button_Save"), 
																	width, height,
																	new BMessage(BMM_SAVE), this, 
																	"Save mail as draft (for later use)"),
					mNewButton = new BmToolbarButton( "New", 
																 TheResources->IconByName("Button_New"), 
																 width, height,
																 new BMessage(BMM_NEW_MAIL), this, 
																 "Compose a new mail message"),
					mAttachButton = new BmToolbarButton( "Attach", 
																	 TheResources->IconByName("Attachment"), 
																	 width, height,
																	 new BMessage(BMM_ATTACH), this, 
																	 "Attach a file to this mail"),
					mPeopleButton = new BmToolbarButton( "People", 
																	 TheResources->IconByName("Person"), 
																	 width, height,
																	 new BMessage(BMM_SHOW_PEOPLE), this, 
																	 "Show people information (addresses)"),
					mPrintButton = new BmToolbarButton( "Print", 
																	TheResources->IconByName("Button_Print"), 
																	width, height,
																	new BMessage(BMM_PRINT), this, 
																	"Print selected messages(s)"),
					new Space(),
					0
				)
			),
			new Space(minimax(-1,4,-1,4)),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mFromControl = new BmTextControl( "From:", true),
				mSmtpControl = new BmMenuControl( "SMTP-Server:", new BPopUpMenu( ""), 0.4),
				0
			),
			new HGroup(
				mShowDetails1Button = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_SHOWDETAILS1), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mToControl = new BmPeopleControl( "To:"),
				mCharsetControl = new BmMenuControl( "Charset:", new BPopUpMenu( ""), 0.4),
				0
			),
			mDetails1Group = new VGroup(
				new HGroup(
					mShowDetails2Button = 
					new MPictureButton( minimax( 16,16,16,16), 
											  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
											  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
											  new BMessage( BM_SHOWDETAILS2), this, B_TWO_STATE_BUTTON),
					new Space(minimax(4,-1,4,-1)),
					mCcControl = new BmPeopleControl( "Cc:"),
					mReplyToControl = new BmTextControl( "Reply-To:", false),
					0
				),
				mDetails2Group = new HGroup(
					new Space(minimax(20,-1,20,-1)),
					mBccControl = new BmPeopleControl( "Bcc:"),
					mSenderControl = new BmTextControl( "Sender:", false),
					0
				),
				0
			),
			mSubjectGroup = new HGroup(
				mShowDetails3Button = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_SHOWDETAILS3), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", false),
				0
			),
			mDetails3Group = new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mSignatureControl = new BmMenuControl( "Signature:", new BPopUpMenu( "")),
				new Space(minimax(20,-1,20,-1)),
				mEditHeaderControl = new BmCheckControl( "Edit Headers Before Send", 1, false),
				new Space(),
				0
			),
			mSeparator = new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);

	mSendButton->AddActionVariation( "Send Now", new BMessage(BMM_SEND_NOW));
	mSendButton->AddActionVariation( "Send Later", new BMessage(BMM_SEND_LATER));

	float divider = mToControl->Divider();
	divider = MAX( divider, mSubjectControl->Divider());
	divider = MAX( divider, mFromControl->Divider());
	divider = MAX( divider, mCcControl->Divider());
	divider = MAX( divider, mBccControl->Divider());
	divider = MAX( divider, mReplyToControl->Divider());
	divider = MAX( divider, mSenderControl->Divider());
	divider = MAX( divider, mSignatureControl->Divider());
	mToControl->SetDivider( divider);
	mSubjectControl->SetDivider( divider);
	mFromControl->SetDivider( divider);
	mCcControl->SetDivider( divider);
	mBccControl->SetDivider( divider);
	mReplyToControl->SetDivider( divider);
	mSenderControl->SetDivider( divider);
	mSignatureControl->SetDivider( divider);

	divider = MAX( 0, mSmtpControl->Divider());
	divider = MAX( divider, mCharsetControl->Divider());
	mSmtpControl->SetDivider( divider);
	mCharsetControl->SetDivider( divider);
	mEditHeaderControl->ct_mpm = mSmtpControl->ct_mpm;

	mShowDetails1Button->SetFlags( mShowDetails1Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails2Button->SetFlags( mShowDetails2Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails3Button->SetFlags( mShowDetails3Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));

	// initially, the detail-parts are hidden:
	if (!mShowDetails2)
		mDetails2Group->RemoveSelf();
	if (!mShowDetails1)
		mDetails1Group->RemoveSelf();
	if (!mShowDetails3)
		mDetails3Group->RemoveSelf();

	// add all identities to from menu twice (one for single-address mode, one for adding addresses):
	mFromControl->Menu()->SetLabelFromMarked( false);
	BmModelItemMap::const_iterator iter;
	BMenu* subMenu = new BMenu( "Add...");
	vector<BmString> aliases;
	for( iter = TheIdentityList->begin(); iter != TheIdentityList->end(); ++iter) {
		BmIdentity* ident = dynamic_cast< BmIdentity*>( iter->second.Get());
		mFromControl->Menu()->AddItem( new BMenuItem( ident->Key().String(), new BMessage( BM_FROM_SET)));
		subMenu->AddItem( new BMenuItem( ident->Key().String(), new BMessage( BM_FROM_ADDED)));
	}
	mFromControl->Menu()->AddItem( subMenu);

	// add all smtp-accounts to smtp menu:
	for( iter = TheSmtpAccountList->begin(); iter != TheSmtpAccountList->end(); ++iter) {
		BmSmtpAccount* acc = dynamic_cast< BmSmtpAccount*>( iter->second.Get());
		mSmtpControl->Menu()->AddItem( new BMenuItem( acc->Key().String(), new BMessage( BM_SMTP_SELECTED)));
		if (TheSmtpAccountList->size() == 1)
			mSmtpControl->MarkItem( acc->Key().String());
	}
	
	// add all charsets to menu:
	AddCharsetMenu( mCharsetControl->Menu(), this, BM_CHARSET_SELECTED);

	// add all signatures to signature menu:
	mSignatureControl->Menu()->AddItem( new BMenuItem( "<none>", new BMessage( BM_SIGNATURE_SELECTED)));
	for( iter = TheSignatureList->begin(); iter != TheSignatureList->end(); ++iter) {
		BmSignature* sig = dynamic_cast< BmSignature*>( iter->second.Get());
		mSignatureControl->Menu()->AddItem( new BMenuItem( sig->Key().String(), new BMessage( BM_SIGNATURE_SELECTED)));
	}

	mSaveButton->SetEnabled( mModified);
	mMailView->SetModificationMessage( new BMessage( BM_TEXTFIELD_MODIFIED));

	// watch changes to bodypartview in order to be set the changed-flag accordingly:	
	mMailView->BodyPartView()->StartWatching( this, BM_NTFY_LISTCONTROLLER_MODIFIED);

	// temporarily disabled:
	mPeopleButton->SetEnabled( false);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailEditWin::UnarchiveState( BMessage* archive) {
	status_t ret = inherited::UnarchiveState( archive);
	if (ret == B_OK) {
		BRect frame = Frame();
		if (nNextXPos != frame.left || nNextYPos != frame.top) {
			nNextXPos = frame.left;
			nNextYPos = frame.top;
		} else {
			nNextXPos += 10;
			nNextYPos += 16;
			if (nNextYPos > 300) {
				nNextXPos = 300;
				nNextYPos = 100;
			}
		}
		BRect scrFrame = bmApp->ScreenFrame();
		frame.bottom = MIN( frame.bottom, scrFrame.bottom-5);
		frame.right = MIN( frame.right, scrFrame.right-5);
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( frame.Width(), frame.Height());
		WriteStateInfo();
	} else {
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( 400, 400);
		WriteStateInfo();
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
MMenuBar* BmMailEditWin::CreateMenu() {
	MMenuBar* menubar = new MMenuBar();
	BMenu* menu = NULL;
	// File
	menu = new BMenu( "File");
	menu->AddItem( CreateMenuItem( "Save", BMM_SAVE, "SaveMail"));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Preferences...", BMM_PREFERENCES), bmApp);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), bmApp);
	menubar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( CreateMenuItem( "Undo", B_UNDO));
	menu->AddItem( CreateMenuItem( "Redo", B_REDO));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Cut", B_CUT));
	menu->AddItem( CreateMenuItem( "Copy", B_COPY));
	menu->AddItem( CreateMenuItem( "Paste", B_PASTE));
	menu->AddItem( CreateMenuItem( "Select All", B_SELECT_ALL));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Find...", BMM_FIND));
	menu->AddItem( CreateMenuItem( "Find Next", BMM_FIND_NEXT));
	menubar->AddItem( menu);

	// Network
	menu = new BMenu( "Network");
	menu->AddItem( CreateMenuItem( "Send Mail Now", BMM_SEND_NOW));
	menu->AddItem( CreateMenuItem( "Send Mail Later", BMM_SEND_LATER));
	menu->AddSeparatorItem();
	menubar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( CreateMenuItem( "New Message", BMM_NEW_MAIL));
	menubar->AddItem( menu);

	// temporary deactivations:
	menubar->FindItem( BMM_FIND)->SetEnabled( false);
	menubar->FindItem( BMM_FIND_NEXT)->SetEnabled( false);

	return menubar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMailEditWin::CreateMailView( minimax minmax, BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, true);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::MessageReceived( BMessage* msg) {
	Regexx rx;
	try {
		switch( msg->what) {
			case BM_SHOWDETAILS1: {
				int32 newVal = (mShowDetails1 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails1Button->SetValue( newVal);
				mShowDetails1 = newVal != B_CONTROL_OFF;
				if (mShowDetails1)
					mOuterGroup->AddChild( mDetails1Group, mSubjectGroup);
				else
					mDetails1Group->RemoveSelf();
				RecalcSize();
//				mShowDetails2Button->Invoke();
				break;
			}
			case BM_SHOWDETAILS2: {
				int32 newVal = (mShowDetails2 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails2Button->SetValue( newVal);
				mShowDetails2 = newVal != B_CONTROL_OFF;
				if (mShowDetails2)
					mDetails1Group->AddChild( mDetails2Group);
				else
					mDetails2Group->RemoveSelf();
				RecalcSize();
				break;
			}
			case BM_SHOWDETAILS3: {
				int32 newVal = (mShowDetails3 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails3Button->SetValue( newVal);
				mShowDetails3 = newVal != B_CONTROL_OFF;
				if (mShowDetails3)
					mOuterGroup->AddChild( mDetails3Group, mSeparator);
				else
					mDetails3Group->RemoveSelf();
				RecalcSize();
				break;
			}
			case BMM_SEND_LATER:
			case BMM_SEND_NOW: {
				BM_LOG2( BM_LogMailEditWin, BmString("Asked to send mail"));
				if (!SaveMail( true))
					break;
				BM_LOG2( BM_LogMailEditWin, "...mail was saved");
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (!mail)
					break;
				if (mail->IsFieldEmpty( mail->IsRedirect() ? BM_FIELD_RESENT_FROM : BM_FIELD_FROM)) {
					BM_SHOWERR("Please enter at least one address into the <FROM> field before sending this mail, thank you.");
					break;
				}
				if (mail->IsFieldEmpty( mail->IsRedirect() ? BM_FIELD_RESENT_TO : BM_FIELD_TO) 
				&& mail->IsFieldEmpty( mail->IsRedirect() ? BM_FIELD_RESENT_CC : BM_FIELD_CC)
				&& mail->IsFieldEmpty( mail->IsRedirect() ? BM_FIELD_RESENT_BCC : BM_FIELD_BCC)) {
					BM_SHOWERR("Please enter at least one address into the\n\t<TO>,<CC> or <BCC>\nfield before sending this mail, thank you.");
					break;
				}
				if (msg->what == BMM_SEND_NOW) {
					BmRef<BmListModelItem> smtpRef = TheSmtpAccountList->FindItemByKey( mail->AccountName());
					BmSmtpAccount* smtpAcc = dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
					if (smtpAcc) {
						if (mEditHeaderControl->Value()) {
							// allow user to edit mail-header before we send it:
							BRect screen( bmApp->ScreenFrame());
							float w=600, h=400;
							BRect alertFrame( (screen.Width()-w)/2,(screen.Height()-h)/2,
													(screen.Width()+w)/2,(screen.Height()+h)/2);
							BmString headerStr;
							headerStr.ConvertLinebreaksToLF( &mail->Header()->HeaderString());
							TextEntryAlert* alert = 
								new TextEntryAlert( "Edit Headers", 
														  "Please edit the mail-headers below:",
														  headerStr.String(),
														  "Cancel",
														  "OK, Send Message",
														  false, 80, 20, B_WIDTH_FROM_LABEL, true,
														  &alertFrame
								);
							alert->SetShortcut( B_ESCAPE, 0);
							alert->TextEntryView()->DisallowChar( 27);
							alert->TextEntryView()->SetFontAndColor( be_fixed_font);
							alert->Go( new BInvoker( new BMessage( BM_EDIT_HEADER_DONE), BMessenger( this)));
							break;
						} else {
							BM_LOG2( BM_LogMailEditWin, "...marking mail as pending");
							mail->MarkAs( BM_MAIL_STATUS_PENDING);
							smtpAcc->mMailVect.push_back( mail);
							BM_LOG2( BM_LogMailEditWin, "...passing mail to smtp-account");
							TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
						}
					} else {
						ShowAlertWithType( "Before you can send this mail, you have to select the SMTP-Account to use for sending it.",
												 B_INFO_ALERT);
						break;
					}
				} else 
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
				PostMessage( B_QUIT_REQUESTED);
				break;
			}
			case BM_EDIT_HEADER_DONE: {
				// User is done with editing the mail-header. We reconstruct the mail with
				// the new header and then send it:
				BmRef<BmMail> mail = mMailView->CurrMail();
				int32 result;
				const char* headerStr; 
				if (!mail || msg->FindInt32( "which", &result) != B_OK 
				|| msg->FindString( "entry_text", &headerStr) != B_OK || result != 1)
					break;
				BmRef<BmListModelItem> smtpRef = TheSmtpAccountList->FindItemByKey( mail->AccountName());
				BmSmtpAccount* smtpAcc = dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
				if (smtpAcc) {
					mail->SetNewHeader( headerStr);
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
					smtpAcc->mMailVect.push_back( mail);
					TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
					PostMessage( B_QUIT_REQUESTED);
				}
				break;
			}
			case BMM_SAVE: {
				SaveMail( false);
				break;
			}
			case BM_TO_CC_BCC_ADDED: {
				BmTextControl* cntrl;
				if (msg->FindPointer( MSG_CONTROL, (void**)&cntrl) == B_OK) {
					BmString email = msg->FindString( MSG_ADDRESS);
					AddAddressToTextControl( cntrl, email);
				}
				break;
			}
			case BM_TO_CC_BCC_CLEAR: {
				BmTextControl* cntrl;
				if (msg->FindPointer( MSG_CONTROL, (void**)&cntrl) == B_OK) {
					cntrl->SetText( "");
					cntrl->TextView()->Select( 0, 0);
					cntrl->TextView()->ScrollToSelection();
				}
				break;
			}
			case BM_FROM_SET:
			case BM_FROM_ADDED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					BmRef<BmListModelItem> identRef = TheIdentityList->FindItemByKey( item->Label());
					BmIdentity* ident = dynamic_cast< BmIdentity*>( identRef.Get()); 
					if (!ident)
						break;
					TheIdentityList->CurrIdentity( ident);
					BmString fromString = msg->what == BM_FROM_ADDED ? mFromControl->Text() : "";
					if (rx.exec( fromString, "\\S+")) {
						fromString << ", " << ident->GetFromAddress();
					} else {
						fromString << ident->GetFromAddress();
					}
					mFromControl->SetText( fromString.String());
					mFromControl->TextView()->Select( fromString.Length(), fromString.Length());
					mFromControl->TextView()->ScrollToSelection();
					if (msg->what == BM_FROM_SET) {
						// mark selected identity:
						item->SetMarked( true);
						// select corresponding smtp-account, if any:
						mSmtpControl->MarkItem( ident->SMTPAccount().String());
						// update signature:
						BmString sigName = ident->SignatureName();
						mMailView->SetSignatureByName( sigName);
						if (sigName.Length())
							mSignatureControl->MarkItem( sigName.String());
						else
							mSignatureControl->MarkItem( "<none>");
					}
				}
				break;
			}
			case BMM_ATTACH: {
				entry_ref attachRef;
				if (msg->FindRef( "refs", 0, &attachRef) != B_OK) {
					// first step, let user select files to attach:
					if (!mAttachPanel) {
						mAttachPanel = new BFilePanel( B_OPEN_PANEL, new BMessenger(this), NULL,
																 B_FILE_NODE, true, msg);
					}
					mAttachPanel->Show();
				} else {
					// second step, attach selected files to mail:
					mMailView->BodyPartView()->AddAttachment( msg);
				}
				break;
			}
			case BM_SIGNATURE_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					BmString sigName = item->Label();
					if (sigName=="<none>")
						mMailView->SetSignatureByName( "");
					else
						mMailView->SetSignatureByName( sigName);
					mModified = true;
					mSaveButton->SetEnabled( true);
				}
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BControl* source;
				if (msg->FindPointer( "source", (void**)&source)==B_OK
				&& source==mSubjectControl) {
					SetTitle( (BmString("Edit Mail: ") + mSubjectControl->Text()).String());
				}
				mModified = true;
				mSaveButton->SetEnabled( true);
				break;
			}
			case BM_CHARSET_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					mCharsetControl->ClearMark();
					mCharsetControl->MenuItem()->SetLabel( item->Label());
					item->SetMarked( true);
					mModified = true;
					mSaveButton->SetEnabled( true);
				}
				break;
			}
			case BM_SMTP_SELECTED:
			case B_OBSERVER_NOTICE_CHANGE: {
				mModified = true;
				mSaveButton->SetEnabled( true);
				break;
			}
			case BMM_NEW_MAIL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailEditWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddAddressToTextControl( BmTextControl* cntrl, email)
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::AddAddressToTextControl( BmTextControl* cntrl, 
															const BmString& email) {
	if (cntrl) {
		Regexx rx;
		BmString currStr = cntrl->Text();
		if (rx.exec( currStr, "\\S+"))
			currStr << ", " << email;
		else
			currStr << email;
		cntrl->SetText( currStr.String());
		cntrl->TextView()->Select( currStr.Length(), currStr.Length());
		cntrl->TextView()->ScrollToSelection();
	}
}

/*------------------------------------------------------------------------------*\
	BeginLife()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::BeginLife() {
}

/*------------------------------------------------------------------------------*\
	Show()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::Show() {
	if (!Looper()->IsLocked()) {
		// showing living window, we bring it to front:
		LockLooper();
		if (IsMinimized())
			Minimize( false);
		inherited::Hide();
		inherited::Show();
		UnlockLooper();
	} else
		inherited::Show();
}

/*------------------------------------------------------------------------------*\
	EditMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::EditMail( BmMailRef* ref) {
	mMailView->ShowMail( ref, false);
							// false=>synchronize (i.e. wait till mail is being displayed)
	BmRef<BmMail> mail = mMailView->CurrMail();
	SetFieldsFromMail( mail.Get());
}

/*------------------------------------------------------------------------------*\
	EditMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::EditMail( BmMail* mail) {
	mMailView->ShowMail( mail, false);
							// false=>synchronize (i.e. wait till mail is being displayed)
	SetFieldsFromMail( mail);
}

/*------------------------------------------------------------------------------*\
	CurrMail()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMailEditWin::CurrMail() const { 
	return mMailView->CurrMail();
}

/*------------------------------------------------------------------------------*\
	SetFieldFromMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetFieldsFromMail( BmMail* mail) {
	if (mail) {
		BmString fromAddrSpec;
		if (mail->IsRedirect()) {
			mBccControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_RESENT_BCC).String());
			mCcControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_RESENT_CC).String());
			mFromControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_RESENT_FROM).String());
			mSenderControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_RESENT_SENDER).String());
			mToControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_RESENT_TO).String());
			fromAddrSpec 
				= mail->Header()->GetAddressList( BM_FIELD_RESENT_FROM).FirstAddress().AddrSpec();
		} else {
			mBccControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_BCC).String());
			mCcControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_CC).String());
			mFromControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_FROM).String());
			mSenderControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_SENDER).String());
			mToControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_TO).String());
			mReplyToControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_REPLY_TO).String());
			fromAddrSpec 
				= mail->Header()->GetAddressList( BM_FIELD_FROM).FirstAddress().AddrSpec();
		}
		mSubjectControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_SUBJECT).String());
		SetTitle( (BmString("Edit Mail: ") + mSubjectControl->Text()).String());
		// mark corresponding identity:
		BmRef<BmIdentity> identRef = TheIdentityList->FindIdentityForAddrSpec( fromAddrSpec);
		if (identRef) {
			BMenuItem* item = mFromControl->Menu()->FindItem( identRef->Key().String());
			if (item)
				item->SetMarked( true);
		}
		// mark corresponding SMTP-account (if any):
		BmString smtpAccount = mail->AccountName();
		mSmtpControl->MarkItem( smtpAccount.String());
		// mark signature of current mail as selected:
		BmString sigName = mail->SignatureName();
		if (sigName.Length())
			mSignatureControl->MarkItem( sigName.String());
		else
			mSignatureControl->MarkItem( "<none>");
		// mark corresponding charset:
		BmString charset = mail->DefaultCharset();
		charset.ToLower();
		mCharsetControl->MenuItem()->SetLabel( charset.String());
		mCharsetControl->MarkItem( charset.String());
		// try to set convenient focus:
		if (!mFromControl->TextView()->TextLength())
			mFromControl->MakeFocus( true);
		else if (!mToControl->TextView()->TextLength())
			mToControl->MakeFocus( true);
		else if (!mSubjectControl->TextView()->TextLength())
			mSubjectControl->MakeFocus( true);
		else
			mMailView->MakeFocus( true);
		// now make certain fields visible if they contain values:
		if (BmString(mCcControl->Text()).Length() || BmString(mBccControl->Text()).Length()) {
			mShowDetails1Button->SetValue( B_CONTROL_ON);
			mShowDetails1Button->SetTarget( BMessenger( this));
			mShowDetails1Button->Invoke();
		}
		if (BmString(mBccControl->Text()).Length()) {
			mShowDetails2Button->SetValue( B_CONTROL_ON);
			mShowDetails2Button->SetTarget( BMessenger( this));
			mShowDetails2Button->Invoke();
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateMailFromFields()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::CreateMailFromFields( bool hardWrapIfNeeded) {
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		BmString editedText;
		mMailView->GetWrappedText( editedText, hardWrapIfNeeded);
		BmString charset = mCharsetControl->MenuItem()->Label();
		if (!charset.Length())
			charset = ThePrefs->GetString( "DefaultCharset");
		BMenuItem* smtpItem = mSmtpControl->Menu()->FindMarked();
		BmString smtpAccount = smtpItem ? smtpItem->Label() : "";
		mail->AccountName( smtpAccount);
		if (mail->IsRedirect()) {
			mail->SetFieldVal( BM_FIELD_RESENT_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_TO, mToControl->Text());
		} else {
			mail->SetFieldVal( BM_FIELD_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_TO, mToControl->Text());
			mail->SetFieldVal( BM_FIELD_REPLY_TO, mReplyToControl->Text());
		}
		mail->SetFieldVal( BM_FIELD_SUBJECT, mSubjectControl->Text());
		if (!mail->IsRedirect() 
		&& ThePrefs->GetBool( "SetMailDateWithEverySave", true)) {
			mail->SetFieldVal( BM_FIELD_DATE, TimeToString( time( NULL), 
																			"%a, %d %b %Y %H:%M:%S %z"));
		}
		try {
			bool res = mail->ConstructRawText( editedText, charset, smtpAccount);
			return res;
		} catch( BM_text_error& textErr) {
			if (textErr.posInText >= 0) {
				int32 end = 1+textErr.posInText;
				while( IS_WITHIN_UTF8_MULTICHAR( mMailView->ByteAt( end)))
					end++;
				mMailView->Select( textErr.posInText, end);
				mMailView->ScrollToSelection();
			}
			ShowAlertWithType( textErr.what(), B_WARNING_ALERT);
			return false;
		}
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	SaveMail( saveForSend)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::SaveMail( bool saveForSend) {
	if (!saveForSend && !mModified && !mHasNeverBeenSaved)
		return true;
	if (!CreateMailFromFields( saveForSend))
		return false;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		mail->Outbound( true);				// just to make sure... >:o)
		if (saveForSend) {
			// set 'out'-folder as default and then start filter-job:
			mail->SetDestFoldername( BM_MAIL_FOLDER_OUT);
			mail->ApplyFilter();
		} else {
			// drop draft mails into 'draft'-folder:
			if (mail->Status() == BM_MAIL_STATUS_DRAFT)
				mail->SetDestFoldername( BM_MAIL_FOLDER_DRAFT);
		}
			
		if (mail->Store()) {
			mHasNeverBeenSaved = false;
			mModified = false;
			if (LockLooper()) {
				mSaveButton->SetEnabled( false);
				UnlockLooper();
			}
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::QuitRequested() {
	BM_LOG2( BM_LogMailEditWin, BmString("MailEditWin has been asked to quit"));
	if (mModified) {
		if (IsMinimized())
			Minimize( false);
		Activate();
		BAlert* alert = new BAlert( "title", "Save mail as draft before closing?",
											 "Cancel", "Don't Save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut( 0, B_ESCAPE);
		int32 result = alert->Go();
		switch( result) {
			case 0:
				return false;
			case 1: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail) {
					// reset mail to original values:
					mail->ResyncFromDisk();
				}
				break;
			}
			case 2:
				return SaveMail( false);
		}
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMailEditWin::Quit() {
	mMailView->WriteStateInfo();
	mMailView->DetachModel();
	BM_LOG2( BM_LogMailEditWin, BmString("MailEditWin has quit"));
	inherited::Quit();
}
