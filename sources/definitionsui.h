/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Authors.
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
#include <qdir.h>


namespace Icons { //New Emoji Icons.
inline constexpr char Close[]         = "❌️";    inline constexpr char History[]       = "🕙️";    inline constexpr char Info[]          = "ℹ️";
inline constexpr char File[]          = "📄️";    inline constexpr char Transfer[]      = "📥️";
inline constexpr char Smiley[]        = "🤪️";    inline constexpr char Save[]          = "💾️";
inline constexpr char Stop[]          = "🚫️";    inline constexpr char Folder[]        = "📁️";    inline constexpr char ShowFolder[]    = "📂️";
inline constexpr char Broadcast[]     = "📢️";    inline constexpr char Plus[]          = "➕️";    inline constexpr char Refresh[]       = "♻️";
inline constexpr char Web[]           = "🌍️";    inline constexpr char Chat[]          = "💬️";    inline constexpr char Play[]          = "▶️️️️";
inline constexpr char Public[]        = "👨‍👩‍👧‍👦️";    inline constexpr char Alert[]         = "⚠️";    inline constexpr char VoiceSet[]      = "🎙️️";
inline constexpr char Microphone[]    = "🎤️️";    inline constexpr char Speaker[]       = "🎧️️";    inline constexpr char Telephone[]     = "📞️️";
inline constexpr char Camera[]        = "📷️️️";    inline constexpr char Camcorder[]     = "📹️️";    inline constexpr char Nudge[]         = "🛎️️️";
inline constexpr char Settings[]      = "⚙️";    inline constexpr char GeneralSet[]    = "🖥️";    inline constexpr char AccountSet[]    = "👨‍👩‍👦️";
inline constexpr char MessageSet[]    = "📨️";    inline constexpr char HistorySet[]    = "🕙️";    inline constexpr char AlertSet[]      = "⚠️";
inline constexpr char NetworkSet[]    = "🔗️";    inline constexpr char TransferSet[]   = "📥️";    inline constexpr char ThemeSet[]      = "🎨️";
inline constexpr char HotkeySet[]     = "⌨️";    inline constexpr char HangUp[]        = "📵";
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
    SE_MutedTalk,
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

const int HTMLESC_COUNT = 5;
const QString htmlSymbol[] = {"&", "\"", "<", ">", " "};
const QString htmlEscape[] = {"&amp;", "&quot;", "&lt;", "&gt;", "&nbsp;"};

enum InfoType {
    IT_Ok			= 0x00,
    IT_Busy			= 0x01,
    IT_Offline		= 0x02,
    IT_Disconnected	= 0x04,
    IT_Away			= 0x08
};

/****************************************************************************
**	Chat state definitions
**	The enum and the string array should always be synced
****************************************************************************/
enum ChatState {
    CS_Blank = 0,
    CS_Active,
    CS_Composing,
    CS_Paused,
    CS_Inactive,
    CS_Gone,
    CS_Max
};

const QString ChatStateNames[] = {
    "",
    "active",
    "composing",
    "paused",
    "inactive",
    "gone"
};

struct Templates {
    static QString load(const char* path) {
        QFile f(path); return f.open(QFile::ReadOnly) ? f.readAll() : "";
    }

    QString inMsg      = load(":/templates/old_content.html");
    QString inNextMsg  = load(":/templates/old_nextcontent.html");
    QString pubMsg     = load(":/templates/old_broadcast.html");
    QString sysMsg     = load(":/templates/old_status.html");
    QString sysNextMsg = load(":/templates/old_nextstatus.html");
    QString reqMsg     = load(":/templates/old_request.html");
    QString stateMsg   = load(":/templates/old_status.html");
};

#endif // DEFINITIONSUI_H
