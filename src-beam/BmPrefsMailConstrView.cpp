/*
	BmPrefsMailConstrView.cpp
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
#include <PopUpMenu.h>

#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMenuControl.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmPrefsMailConstrView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsMailConstrView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailConstrView::BmPrefsMailConstrView() 
	:	inherited( "Sending Mail")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Formatting",
					new VGroup(
						mMaxLineLenControl = new BmTextControl( "Set right margin to (chars):"),
						mHardWrapControl = new BmCheckControl( "Hard-wrap mailtext at right margin", 
																			  new BMessage(BM_HARD_WRAP_CHANGED), 
																			  this, ThePrefs->GetBool("HardWrapMailText")),
						mHardWrapAt78Control = new BmCheckControl( "Always respect maximum line-length of 78 characters", 
																			  new BMessage(BM_HARD_WRAP_AT_78_CHANGED), 
																			  this, ThePrefs->GetInt("MaxLineLenForHardWrap",998)<100),
						new Space( minimax(0,10,0,10)),
						mQuotingStringControl = new BmTextControl( "Quoting string:"),
						mQuoteFormattingControl = new BmMenuControl( "Quote-formatting:", new BPopUpMenu("")),
						new Space( minimax(0,10,0,10)),
						mDefaultEncodingControl = new BmMenuControl( "Default-encoding:", new BPopUpMenu("")),
						new Space(),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Construction Options",
					new VGroup(
						mAllow8BitControl = new BmCheckControl( "Use 8-bit-MIME ", 
																			  new BMessage(BM_ALLOW_8_BIT_CHANGED), 
																			  this, ThePrefs->GetBool("Allow8BitMime")),
						mSpecialForEachBccControl = new BmCheckControl( "Generate header for each Bcc-recipient", 
																					   new BMessage(BM_EACH_BCC_CHANGED), 
																					   this, ThePrefs->GetBool("SpecialHeaderForEachBcc")),
						mPreferUserAgentControl = new BmCheckControl( "Prefer 'UserAgent'-header over 'X-Mailer'", 
																					 new BMessage(BM_PREFER_USER_AGENT_CHANGED), 
																					 this, ThePrefs->GetBool("PreferUserAgentOverX-Mailer")),
						mGenerateIDsControl = new BmCheckControl( "Generate own message-IDs", 
																				new BMessage(BM_GENERATE_MSGIDS_CHANGED), 
																				this, ThePrefs->GetBool("GenerateOwnMessageIDs")),
						mMakeQpSafeControl = new BmCheckControl( "Make quoted-printable safe for non-ASCII gateways (EBCDIC)", 
																			  new BMessage(BM_QP_SAFE_CHANGED), 
																			  this, ThePrefs->GetBool("MakeQPSafeForEBCDIC")),
						new Space(),
						0
					)
				),
				0
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Forwarding",
				new VGroup(
					mForwardIntroStrControl = new BmTextControl( "Intro:"),
					mForwardSubjectStrControl = new BmTextControl( "Subject:"),
					mForwardSubjectRxControl = new BmTextControl( "Regex that checks if forward:"),
					new HGroup( 
						mDefaultForwardTypeControl = new BmMenuControl( "Default forward-type:", new BPopUpMenu("")),
						new Space(),
						0
					),
					new Space( minimax(0,4,0,4)),
					mDontAttachVCardsControl = new BmCheckControl( "Do not attach v-cards in forward", 
																				  new BMessage(BM_ATTACH_VCARDS_CHANGED), 
																				  this, ThePrefs->GetBool("DoNotAttachVCardsToForward")),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Replying",
				new VGroup(
					mReplyIntroStrControl = new BmTextControl( "Intro:"),
					mReplySubjectStrControl = new BmTextControl( "Subject:"),
					mReplySubjectRxControl = new BmTextControl( "Regex that checks if reply:"),
					new Space(),
					0
				)
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mMaxLineLenControl->Divider();
	divider = MAX( divider, mQuotingStringControl->Divider());
	divider = MAX( divider, mQuoteFormattingControl->Divider());
	divider = MAX( divider, mDefaultEncodingControl->Divider());
	divider = MAX( divider, mForwardIntroStrControl->Divider());
	divider = MAX( divider, mForwardSubjectStrControl->Divider());
	divider = MAX( divider, mForwardSubjectRxControl->Divider());
	divider = MAX( divider, mDefaultForwardTypeControl->Divider());
	divider = MAX( divider, mReplyIntroStrControl->Divider());
	divider = MAX( divider, mReplySubjectStrControl->Divider());
	divider = MAX( divider, mReplySubjectRxControl->Divider());
	mMaxLineLenControl->SetDivider( divider);
	mQuotingStringControl->SetDivider( divider);
	mQuoteFormattingControl->SetDivider( divider);
	mDefaultEncodingControl->SetDivider( divider);
	mForwardIntroStrControl->SetDivider( divider);
	mForwardSubjectStrControl->SetDivider( divider);
	mForwardSubjectRxControl->SetDivider( divider);
	mDefaultForwardTypeControl->SetDivider( divider);
	mReplyIntroStrControl->SetDivider( divider);
	mReplySubjectStrControl->SetDivider( divider);
	mReplySubjectRxControl->SetDivider( divider);

	BmString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetText( val.String());
	mQuotingStringControl->SetText( ThePrefs->GetString("QuotingString").String());
	mForwardIntroStrControl->SetText( ThePrefs->GetString("ForwardIntroStr").String());
	mForwardSubjectStrControl->SetText( ThePrefs->GetString("ForwardSubjectStr").String());
	mForwardSubjectRxControl->SetText( ThePrefs->GetString("ForwardSubjectRX").String());
	mReplyIntroStrControl->SetText( ThePrefs->GetString("ReplyIntroStr").String());
	mReplySubjectStrControl->SetText( ThePrefs->GetString("ReplySubjectStr").String());
	mReplySubjectRxControl->SetText( ThePrefs->GetString("ReplySubjectRX").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsMailConstrView::~BmPrefsMailConstrView() {
	TheBubbleHelper.SetHelp( mMaxLineLenControl, NULL);
	TheBubbleHelper.SetHelp( mQuotingStringControl, NULL);
	TheBubbleHelper.SetHelp( mQuoteFormattingControl, NULL);
	TheBubbleHelper.SetHelp( mForwardIntroStrControl, NULL);
	TheBubbleHelper.SetHelp( mForwardSubjectStrControl, NULL);
	TheBubbleHelper.SetHelp( mForwardSubjectRxControl, NULL);
	TheBubbleHelper.SetHelp( mReplyIntroStrControl, NULL);
	TheBubbleHelper.SetHelp( mReplySubjectStrControl, NULL);
	TheBubbleHelper.SetHelp( mReplySubjectRxControl, NULL);
	TheBubbleHelper.SetHelp( mDefaultEncodingControl, NULL);
	TheBubbleHelper.SetHelp( mSpecialForEachBccControl, NULL);
	TheBubbleHelper.SetHelp( mPreferUserAgentControl, NULL);
	TheBubbleHelper.SetHelp( mGenerateIDsControl, NULL);
	TheBubbleHelper.SetHelp( mMakeQpSafeControl, NULL);
	TheBubbleHelper.SetHelp( mDefaultForwardTypeControl, NULL);
	TheBubbleHelper.SetHelp( mDontAttachVCardsControl, NULL);
	TheBubbleHelper.SetHelp( mHardWrapControl, NULL);
	TheBubbleHelper.SetHelp( mHardWrapAt78Control, NULL);
	TheBubbleHelper.SetHelp( mAllow8BitControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::Initialize() {
	inherited::Initialize();

	mMaxLineLenControl->SetTarget( this);
	mQuotingStringControl->SetTarget( this);
	mForwardIntroStrControl->SetTarget( this);
	mForwardSubjectStrControl->SetTarget( this);
	mForwardSubjectRxControl->SetTarget( this);
	mReplyIntroStrControl->SetTarget( this);
	mReplySubjectStrControl->SetTarget( this);
	mReplySubjectRxControl->SetTarget( this);

	TheBubbleHelper.SetHelp( mMaxLineLenControl, "Here you can enter the maximum number of characters\nper line Beam should allow in the mailtext.\nThis corresponds to the right margin in the mail-editor.");
	TheBubbleHelper.SetHelp( mQuotingStringControl, "Here you can enter the string used for quoting.\nThis string will be prepended to every quoted line.");
	TheBubbleHelper.SetHelp( mQuoteFormattingControl, "This menu controls the way Beam formats quoted lines of a reply/forward.\n\n\
Simple:\n\
	Beam will simply prepend the quote-string to every line. Lines that exceed\n\
	the maximum line length will be wrapped around, resulting in a very short line.\n\
	This sooner or later causes ugly mails with a comb-like layout (alternating long \n\
	and short lines).\n\
Push Margin:\n\
	In this mode, Beam will push the right margin of a reply/forward just as needed.\n\
	The original formatting of the mail stays intact, but the mail may exceed 80 chars\n\
	per line (which will annoy recipients using terminal-based mailers).\n\
Auto Wrap:\n\
	This will cause Beam to automatically rewrap blocks of quoted lines in a way that\n\
	tries to keep the paragraph structure of the original mail intact.\n\
\n\
'Auto Wrap' usually gives the best results for normal text, but since it\n\
can lead to unwanted wrapping of structured text (e.g. code), Beam uses \n\
'Push Margin' by default.");
	TheBubbleHelper.SetHelp( mForwardIntroStrControl, "Here you can enter a string that will \nappear at the top of every forwarded mail.\n\
The following macros are supported:\n\
	%D  -  expands to the original mail's date.\n\
	%T  -  expands to the original mail's time.\n\
	%F  -  expands to the sender of the original mail.");
	TheBubbleHelper.SetHelp( mForwardSubjectStrControl, "Here you can influence the subject-string \nBeam generates for a forwarded mail.\n\
The following macros are supported:\n\
	%S  -  expands to the original mail's subject.");
	TheBubbleHelper.SetHelp( mForwardSubjectRxControl, "This string is the regular-expression (perl-style) Beam uses\nto determine whether a given subject indicates\nthat the mail already is a forward.\nThis way subjects like \n\t'Fwd: Fwd: Fwd: fun-stuff'\ncan be avoided.");
	TheBubbleHelper.SetHelp( mReplyIntroStrControl, "Here you can enter a string that will \nappear at the top of every reply.\n\
The following macros are supported:\n\
	%D  -  expands to the original mail's date.\n\
	%T  -  expands to the original mail's time.\n\
	%F  -  expands to the sender of the original mail.");
	TheBubbleHelper.SetHelp( mReplySubjectStrControl, "Here you can influence the subject-string \nBeam generates for a reply.\n\
The following macros are supported:\n\
	%S  -  expands to the original mail's subject.");
	TheBubbleHelper.SetHelp( mReplySubjectRxControl, "This string is the regular-expression (perl-style) Beam uses\nto determine whether a given subject indicates\nthat the mail already is a reply.\nThis way subjects like \n\t'Re: Re: Re: your offer'\ncan be avoided.");
	TheBubbleHelper.SetHelp( mDefaultEncodingControl, "Here you can select the charset-encoding Beam should usually use.");
	TheBubbleHelper.SetHelp( mSpecialForEachBccControl, "Here you can select the way Beam sends mails with Bcc recipients\n\
	\n\
Checked:\n\
	Beam will send separate mails to each Bcc-recipient, each of which will \n\
	contain exactly one Bcc-header (the current recipient). \n\
	This results in more traffic, but has the advantage that each mail actually \n\
	contains the recipients address in the header, making it look less like spam.\n\
Unchecked:\n\
	Beam will send a single mail to all recipients, which does not contain any \n\
	Bcc-headers. \n\
	This results in less network traffic but makes it more likely that the mail\n\
	is filtered right into the spam-folder on the recipient's side.");
	TheBubbleHelper.SetHelp( mPreferUserAgentControl, "Email-clients used to identify themselves in a header-field called 'X-Mailer'.\nLately, the use of a header-field named 'UserAgent' became popular.\nBy checking/unchecking this control you can decide which field Beam should use.");
	TheBubbleHelper.SetHelp( mGenerateIDsControl, "This control determines if Beam should generate the message-IDs\n\
used to uniquely identify every mail on the net.\n\
If unchecked, the SMTP-server will generate these IDs, which is usually ok,\n\
but makes sorting mails by thread less reliable (since mail-threading is \n\
currently not implemented this doesn't apply for now).");
	TheBubbleHelper.SetHelp( mMakeQpSafeControl, "This makes Beam generate quoted-printables that are safe for EBCDIC-gateways\n (if you don't know what EBCDIC is, don't worry and leave this as is).");
	TheBubbleHelper.SetHelp( mDefaultForwardTypeControl, "Here you can select the forwarding-type Beam should use when you press the 'Forward'-button.");
	TheBubbleHelper.SetHelp( mDontAttachVCardsControl, "Checking this causes Beam to NOT include \nvcard-attachments (appended address-info) in a forwarded mail.");
	TheBubbleHelper.SetHelp( mHardWrapControl, "Checking this causes Beam to hard-wrap the mailtext at the given right margin.\n\
This means that the mail will be sent exactly as you see it on screen,\n\
so that every other mail-program will be able to correctly display this mail.\n\
Uncheck this if you want soft-wrapped paragraphs, that will be layouted by\n\
to the receiving mail-client.\n\
Please note that this only concerns the mailtext entered by yourself, \n\
quoted text will *always* be hard-wrapped.\n\n\
Hint: Use soft-wrap only if you know that the receiving mail-program\n\
is able to handle long lines nicely (most modern mailers do, but some\n\
mailing-list software does not).");
	TheBubbleHelper.SetHelp( mHardWrapAt78Control, "Checking this causes Beam to ensure that no single line of a mail\n\
exceeds the length of 78 characters (as suggested by RFC2822).\n\
This will fix the right margin to at most 78 chars.");
	TheBubbleHelper.SetHelp( mAllow8BitControl, "Checking this causes Beam to allow 8-bit characters\ninside a mail-body without encoding them.\n\
This avoids the use of quoted-printables and is usually ok with \n\
modern mail-servers, but it *may* cause problems during transport,\n\
so if you get complaints about strange/missing characters, try unchecking this.");

	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		AddItemToMenu( mDefaultEncodingControl->Menu(), 
							new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_ENCODING_SELECTED)), this);
	}

	// add quote-formattings:
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_AUTO_WRAP, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_PUSH_MARGIN, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);
	AddItemToMenu( mQuoteFormattingControl->Menu(), 
						new BMenuItem( BmMail::BM_QUOTE_SIMPLE, new BMessage(BM_QUOTE_FORMATTING_SELECTED)), 
						this);

	// add forward-types:
	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Attached", new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);
	AddItemToMenu( mDefaultForwardTypeControl->Menu(), 
						new BMenuItem( "Inline", new BMessage(BM_FORWARD_TYPE_SELECTED)), 
						this);
	Update();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::Update() {
	mHardWrapControl->SetValueSilently( ThePrefs->GetBool("HardWrapMailText"));
	mHardWrapAt78Control->SetEnabled( mHardWrapControl->Value());
	mHardWrapAt78Control->SetValueSilently( ThePrefs->GetInt("MaxLineLenForHardWrap",998)<100);
	mQuoteFormattingControl->MarkItem( ThePrefs->GetString("QuoteFormatting", BmMail::BM_QUOTE_AUTO_WRAP).String());
	mDefaultForwardTypeControl->MarkItem( ThePrefs->GetString( "DefaultForwardType").String());
	mDefaultEncodingControl->MarkItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
	mAllow8BitControl->SetValueSilently( ThePrefs->GetBool("Allow8BitMime"));
	mSpecialForEachBccControl->SetValueSilently( ThePrefs->GetBool("SpecialHeaderForEachBcc"));
	mPreferUserAgentControl->SetValueSilently( ThePrefs->GetBool("PreferUserAgentOverX-Mailer"));
	mGenerateIDsControl->SetValueSilently( ThePrefs->GetBool("GenerateOwnMessageIDs"));
	mMakeQpSafeControl->SetValueSilently( ThePrefs->GetBool("MakeQPSafeForEBCDIC"));
	mDontAttachVCardsControl->SetValueSilently( ThePrefs->GetBool("DoNotAttachVCardsToForward"));
	BmString val;
	val << ThePrefs->GetInt("MaxLineLen");
	mMaxLineLenControl->SetTextSilently( val.String());
	mQuotingStringControl->SetTextSilently( ThePrefs->GetString("QuotingString").String());
	mForwardIntroStrControl->SetTextSilently( ThePrefs->GetString("ForwardIntroStr").String());
	mForwardSubjectStrControl->SetTextSilently( ThePrefs->GetString("ForwardSubjectStr").String());
	mForwardSubjectRxControl->SetTextSilently( ThePrefs->GetString("ForwardSubjectRX").String());
	mReplyIntroStrControl->SetTextSilently( ThePrefs->GetString("ReplyIntroStr").String());
	mReplySubjectStrControl->SetTextSilently( ThePrefs->GetString("ReplySubjectStr").String());
	mReplySubjectRxControl->SetTextSilently( ThePrefs->GetString("ReplySubjectRX").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::SaveData() {
	// prefs are already stored by General View
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::UndoChanges() {
	// prefs are already undone by General View
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsMailConstrView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( source == mMaxLineLenControl)
					ThePrefs->SetInt("MaxLineLen", atoi(mMaxLineLenControl->Text()));
				else if ( source == mQuotingStringControl)
					ThePrefs->SetString("QuotingString", mQuotingStringControl->Text());
				else if ( source == mForwardIntroStrControl)
					ThePrefs->SetString("ForwardIntroStr", mForwardIntroStrControl->Text());
				else if ( source == mForwardSubjectStrControl)
					ThePrefs->SetString("ForwardSubjectStr", mForwardSubjectStrControl->Text());
				else if ( source == mForwardSubjectRxControl)
					ThePrefs->SetString("ForwardSubjectRX", mForwardSubjectRxControl->Text());
				else if ( source == mReplyIntroStrControl)
					ThePrefs->SetString("ReplyIntroStr", mReplyIntroStrControl->Text());
				else if ( source == mReplySubjectStrControl)
					ThePrefs->SetString("ReplySubjectStr", mReplySubjectStrControl->Text());
				else if ( source == mReplySubjectRxControl)
					ThePrefs->SetString("ReplySubjectRX", mReplySubjectRxControl->Text());
				NoticeChange();
				break;
			}
			case BM_EACH_BCC_CHANGED: {
				ThePrefs->SetBool("SpecialHeaderForEachBcc", mSpecialForEachBccControl->Value());
				NoticeChange();
				break;
			}
			case BM_PREFER_USER_AGENT_CHANGED: {
				ThePrefs->SetBool("PreferUserAgentOverX-Mailer", mPreferUserAgentControl->Value());
				NoticeChange();
				break;
			}
			case BM_GENERATE_MSGIDS_CHANGED: {
				ThePrefs->SetBool("GenerateOwnMessageIDs", mGenerateIDsControl->Value());
				NoticeChange();
				break;
			}
			case BM_QP_SAFE_CHANGED: {
				ThePrefs->SetBool("MakeQPSafeForEBCDIC", mMakeQpSafeControl->Value());
				NoticeChange();
				break;
			}
			case BM_ATTACH_VCARDS_CHANGED: {
				ThePrefs->SetBool("DoNotAttachVCardsToForward", mDontAttachVCardsControl->Value());
				NoticeChange();
				break;
			}
			case BM_ALLOW_8_BIT_CHANGED: {
				ThePrefs->SetBool("Allow8BitMime", mAllow8BitControl->Value());
				NoticeChange();
				break;
			}
			case BM_HARD_WRAP_AT_78_CHANGED: {
				ThePrefs->SetInt("MaxLineLenForHardWrap", mHardWrapAt78Control->Value() ? 78 : 998);
				NoticeChange();
				break;
			}
			case BM_HARD_WRAP_CHANGED: {
				bool val = mHardWrapControl->Value();
				ThePrefs->SetBool("HardWrapMailText", val);
				mHardWrapAt78Control->SetEnabled( val);
				if (!val)
					mHardWrapAt78Control->SetValue( false);
				NoticeChange();
				break;
			}
			case BM_ENCODING_SELECTED: {
				BMenuItem* item = mDefaultEncodingControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetInt("DefaultEncoding", CharsetToEncoding( item->Label()));
				NoticeChange();
				break;
			}
			case BM_QUOTE_FORMATTING_SELECTED: {
				BMenuItem* item = mQuoteFormattingControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "QuoteFormatting", item->Label());
				NoticeChange();
				break;
			}
			case BM_FORWARD_TYPE_SELECTED: {
				BMenuItem* item = mDefaultForwardTypeControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "DefaultForwardType", item->Label());
				NoticeChange();
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

