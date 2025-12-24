/****************************************************************************
**
** This file is part of LAN Messenger.
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


#ifndef UIDEFINITIONS_H
#define UIDEFINITIONS_H

#include <QString>
#include "definitions.h"
#include "strings.h"


namespace Icons { //New Emoji Icons.
const QString Close         = "❌️";     const QString History       = "🕙️";
const QString Info          = "ℹ️";     const QString File          = "📄️";
const QString Transfer      = "📥️";     const QString Font          = "✒️";
const QString FontColor     = "🖍️";     const QString Smiley        = "🤪️";
const QString Save          = "💾️";     const QString Stop          = "🚫️";
const QString Folder        = "📁️";     const QString ShowFolder    = "📂️";
const QString Broadcast     = "📢️";     const QString Plus          = "➕️";
const QString Refresh       = "♻️";     const QString Web           = "🌍️";
const QString Chat          = "💬️";     const QString Play          = "▶️️️️";
const QString Public        = "👨‍👩‍👧‍👦️";     const QString Settings      = "⚙️";
const QString GeneralSet    = "🖥️";     const QString AccountSet    = "👨‍👩‍👦️";
const QString MessageSet    = "📨️";     const QString HistorySet    = "🕙️";
const QString AlertSet      = "⚠️";     const QString NetworkSet    = "🔗️";
const QString TransferSet   = "📥️";     const QString ThemeSet      = "🎨️";
const QString HotkeySet     = "⌨️";
}

//Remove this group at first opportunity. Still used for message history.
#define IDR_CRITICAL		":/images/critical"
#define IDR_FILE			":/images/file"
#define IDR_CRITICALMSG		":/images/criticalmsg"
#define IDR_BROADCASTMSG	":/images/broadcastmsg"
#define IDR_FILEMSG			":/images/filemsg"
#define IDR_ADDCONTACT		":/images/addcontact"

//	Resource name definitions
#define IDR_APPICON			":/icons/application"
#define IDR_MESSENGER		":/images/messenger"
#define IDR_LOGO			":/images/logo"
#define IDR_LOGOSMALL		":/images/logosmall"
#define IDR_BLANK			":/images/blank"
#define IDR_HGRIP			":/images/hgrip"
#define IDR_VGRIP			":/images/vgrip"

#define IDR_LICENSETEXT		":/text/license"
#define IDR_THANKSTEXT		":/text/thanks"

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
#define IDR_AVAILABLE		":/images/status/online"
#define IDR_AWAY			":/images/status/away"
#define IDR_BUSY			":/images/status/busy"
#define IDR_NODISTURB		":/images/status/nodisturb"
#define IDR_OFFLINE			":/images/status/offline"
#define IDR_AVAILABLE_BBL	":/icons/bubbles/online"
#define IDR_AWAY_BBL		":/icons/bubbles/away"
#define IDR_BUSY_BBL		":/icons/bubbles/busy"
#define IDR_NODISTURB_BBL	":/icons/bubbles/nodisturb"
#define IDR_OFFLINE_BBL		":/icons/bubbles/offline"

//const QString statusDesc[] = {"Available", "Busy", "Do Not Disturb", "Be Right Back", "Away", "Appear Offline"};
const QString statusPic[] = {IDR_AVAILABLE, IDR_BUSY, IDR_NODISTURB, IDR_AWAY, IDR_AWAY, IDR_OFFLINE};
const QString bubblePic[] = {IDR_AVAILABLE_BBL, IDR_BUSY_BBL, IDR_NODISTURB_BBL, IDR_AWAY_BBL, IDR_AWAY_BBL, IDR_OFFLINE_BBL};

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
    "☂️","🤝","⛔","👍","👎","🤞","🙌","🐢","🦇",":/smileys/70",
//                 Repeats for multiple mapping
//    1.   2.   3.    4.   5.   7.   8.    9.  10.   12.      70.
    "🙂","😀","😉","😮","😛","😠","😕","😳","🙁","😐",":/smileys/70"
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
	SE_Max
};

#define SND_NEWMESSAGE		":/sounds/newmessage"
#define SND_USERONLINE		":/sounds/useronline"
#define SND_USEROFFLINE		":/sounds/useroffline"
#define SND_NEWFILE			":/sounds/newfile"
#define SND_FILEDONE		":/sounds/filedone"
#define SND_NEWPUBMESSAGE	SND_NEWMESSAGE

#define SE_COUNT	6
const QString soundFile[] = {SND_NEWMESSAGE, SND_USERONLINE, SND_USEROFFLINE, SND_NEWFILE, SND_FILEDONE, SND_NEWPUBMESSAGE};

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

#endif // UIDEFINITIONS_H
