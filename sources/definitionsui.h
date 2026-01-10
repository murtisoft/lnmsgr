/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Contributors.
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/


#ifndef DEFINITIONSUI_H
#define DEFINITIONSUI_H

#include <QString>
#include "definitions.h"
#include "strings.h"


namespace Icons { //New Emoji Icons.
const QString Close         = "❌️";    const QString History       = "🕙️";    const QString Info          = "ℹ️";
const QString File          = "📄️";    const QString Transfer      = "📥️";    const QString Font          = "✒️";
const QString FontColor     = "🖍️";    const QString Smiley        = "🤪️";    const QString Save          = "💾️";
const QString Stop          = "🚫️";    const QString Folder        = "📁️";    const QString ShowFolder    = "📂️";
const QString Broadcast     = "📢️";    const QString Plus          = "➕️";    const QString Refresh       = "♻️";
const QString Web           = "🌍️";    const QString Chat          = "💬️";    const QString Play          = "▶️️️️";
const QString Public        = "👨‍👩‍👧‍👦️";    const QString Alert         = "⚠️";    const QString VoiceSet      = "🎙️️";
const QString Microphone    = "🎤️️";    const QString Speaker       = "🎧️️";    const QString Telephone     = "📞️️";
const QString Camera        = "📽️️️";    const QString Screenshot    = "📷️️";    const QString Nudge         = "🛎️️️";
const QString Settings      = "⚙️";    const QString GeneralSet    = "🖥️";    const QString AccountSet    = "👨‍👩‍👦️";
const QString MessageSet    = "📨️";    const QString HistorySet    = "🕙️";    const QString AlertSet      = "⚠️";
const QString NetworkSet    = "🔗️";    const QString TransferSet   = "📥️";    const QString ThemeSet      = "🎨️";
const QString HotkeySet     = "⌨️";
}

//	Resource name definitions
#define IDR_APPICON			":/icons/application"
#define IDR_LOGO128			":/images/logo128"
#define IDR_LOGO48          ":/images/logo48"
#define IDR_LOGO16          ":/images/logo16"

//	item data role definitions
enum ItemDataRole {
	IdRole = Qt::UserRole + 1,	// Unique Id of the item
	TypeRole,	//	Whether item represents a Group or a User
	DataRole,	//	Custom data associated with the item
	StatusRole,	//	Status of the User
	AvatarRole,	//	Avatar image of the User
    SubtextRole,//	Subtext to be displayed on the item
    CapsRole    //  The capabilities of the user
};

//	font size definitions
#define FS_SMALL			0
#define FS_MEDIUM			1
#define FS_LARGE			2

#define FS_COUNT	3
//const QString fontSize[] = {"Small text", "Medium text", "Large text"};
const QString fontStyle[] = {"font-size:8.25pt;", "font-size:9pt;", "font-size:10.5pt;"};

//	status image definitions
#define IDR_AVAILABLE		":/images/status_online"
#define IDR_AWAY			":/images/status_away"
#define IDR_BUSY			":/images/status_busy"
#define IDR_NODISTURB		":/images/status_nodisturb"
#define IDR_OFFLINE			":/images/status_offline"

//const QString statusDesc[] = {"Available", "Busy", "Do Not Disturb", "Be Right Back", "Away", "Appear Offline"};
const QString statusPic[] = {IDR_AVAILABLE, IDR_BUSY, IDR_NODISTURB, IDR_AWAY, IDR_AWAY, IDR_OFFLINE};

//	Smiley definitions
#define SM_COUNT		70
#define SM_MAPCOUNT		81

//Tried to match old MSN style smileys to new emojis.
const QString smileyEmoji[] = {
//   1    2     3    4    5     6    7    8     9    10
    "🙂","😀","😉","😮","😛","😎","😠","😕","😳","🙁",
    "😭","😐","🌧️️","😬","🤓","🤒","🥳","🥱","🤔","🤐",
    "🤫","🤨","🙄", "❤️","💔","🍸","🍺","🐱","🐶","🐌",
    "🐑","🌜","⭐","🌞","🌈","👉","👈","👄","🌹","🥀",
    "🕒","🎁","🎂","📷","💡", "☕","📞","📱","🚗","✈️",
    "💻","💰","🎞️", "🎵","🍕","⚽","📧","👦","👧","🏝️",
    "☂️","🤝","⛔","👍","👎","🤞","🙌","🐢","🦇",":/images/smiley_70",
//                 Repeats for multiple mapping
//    1.   2.   3.    4.   5.   7.   8.    9.  10.   12.      70.
    "🙂","😀","😉","😮","😛","😠","😕","😳","🙁","😐",":/images/smiley_70"  //The last one seems like a mistake, but since old clients do it that way...
};    // 70th is the custom icon, and has to stay a png.

const QString smileyCode[] = {
	":-)", ":-D", ";-)", ":-O", ":-P", "(H)", ":-@", ":-S", ":-$", ":-(", 
	":'(", ":-|", "(ST)", "8o|", "8-|", "+o(", "<:o)", "|-)", "*-)", ":-#", 
	":-*", "^o)", "8-)", "(L)", "(U)", "(D)", "(B)", "(@)", "(&)", "(SN)", 
	"(BAH)", "(S)", "(*)", "(#)", "(R)", "({)", "(})", "(K)", "(F)", "(W)", 
	"(O)", "(G)", "(^)", "(P)", "(I)", "(C)", "(T)", "(MP)", "(AU)", "(AP)", 
	"(CO)", "(MO)", "(~)", "(8)", "(PI)", "(SO)", "(E)", "(Z)", "(X)", "(IP)", 
	"(UM)", "(OK)", "(SS)", "(Y)", "(N)", "(YN)", "(H5)", "(TU)", ":-[", "(M)",
	":)", ":D", ";)", ":O", ":P", ":@", ":S", ":$", ":(", ":|", ":["};

//	Avatar definitions
#define AVT_COUNT	20

//Tried to pick distinct avatar replacements, transparency causes no issues.
const QString avtEmoji[] = {
    "🌅","🌉","🌆","🏜️",
    "🛣️","🏕️","⛄","🏖",
    "🚀","🛸","🚂","⛵",
    "🏀","⚽","⚾","🏈",
    "👨","👨‍🦰","👩","👩‍🦰",
    "👤"
};// Last is the hidden default.

#define AVT_WIDTH		48
#define AVT_HEIGHT		48

//	Sound events definitions
enum SoundEvent {
	SE_NewMessage = 0,
	SE_UserOnline,
	SE_UserOffline,
	SE_NewFile,
	SE_FileDone,
	SE_NewPubMessage,
    SE_MicOn,
    SE_MicOff,
    SE_CamOn,
    SE_CamOff,
    SE_SpeakerOn,
    SE_SpeakerOff,
    SE_RingIn,
    SE_RingOut,
    SE_Nudge,
    SE_MuteTalk,
	SE_Max
};

#define SE_COUNT	16
const QString soundFile[] = {
    ":/sounds/newmessage",      //New Message
    ":/sounds/useronline",      //User Online
    ":/sounds/useroffline",     //User Offline
    ":/sounds/newfile",         //New File Transfer
    ":/sounds/filedone",        //File Transfer Done
    ":/sounds/newmessage",      //New Public Message
    ":/sounds/mic_on",          //Microphone On
    ":/sounds/mic_off",         //Microphone Off
    ":/sounds/cam_on",          //Camera On
    ":/sounds/cam_off",         //Camera Off
    ":/sounds/speaker_on",      //Speaker On
    ":/sounds/speaker_off",     //Speaker Off
    ":/sounds/ring_in",         //Incoming Call
    ":/sounds/ring_out",        //Outgoing Call
    ":/sounds/nudge",           //Nudge
    ":/sounds/muted_talk",      //Talking While Microphone Muted
};

#define AT_COUNT	8
const int awayTimeVal[] = {5, 10, 15, 20, 30, 45, 60, 0};

//	User list views
enum UserListView {
	ULV_Detailed = 0,
	ULV_Compact,
	ULV_Max
};

#define ULV_COUNT	2
const int itemViewHeight[] = {36, 20};

#define RTL_LAYOUT			"RTL"

#ifdef Q_OS_MAC
#define GRAY_TEXT_COLOR     QApplication::palette().color(QPalette::Shadow).darker(175)
#else
#define GRAY_TEXT_COLOR     QApplication::palette().color(QPalette::Shadow)
#endif

#endif // DEFINITIONSUI_H
