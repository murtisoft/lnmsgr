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

#ifndef HOTKEYHANDLER_H
#define HOTKEYHANDLER_H

#include <array>
#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QKeySequence>
#include <QString>
#include "definitionssettings.h"

//	Global (system wide) hotkeys. They fire even when the application has no focus.
//	Registration is platform specific, currently only Windows is implemented
//	(RegisterHotKey). On other platforms isSupported() returns false and nothing is registered.
//
//	Everything here must be used from the GUI thread.
class lmHotkeyHandler : public QObject, public QAbstractNativeEventFilter {
	Q_OBJECT

public:
	enum Action {
		HK_Mic,
		HK_Speaker,
		HK_Camera,
		HK_Max
	};

	explicit lmHotkeyHandler(QObject* parent = nullptr);
	~lmHotkeyHandler(void) override;

	//	Can this platform register global hotkeys at all?
	static bool isSupported(void);
	//	Localized, human readable name of an action, for messages and labels
	static QString actionName(Action action);
	static QKeySequence defaultSequence(Action action);
	//	Empty string if the sequence can be used, otherwise a localized reason why not.
	//	An empty sequence is valid, it means "no hotkey".
	static QString validate(const QKeySequence& sequence);
	//	Read / write one hotkey from / to the settings. Handles defaults and the "unassigned" state.
	static QKeySequence loadSequence(const QSettings* pSettings, Action action);
	static void saveSequence(lmSettingsBase* pSettings, Action action, const QKeySequence& sequence);

	//	Reads the enabled flag and all key sequences, and (re)registers them if anything changed.
	void loadSettings(const QSettings* pSettings);
	//	Temporarily release all hotkeys, e.g. while the user is recording a new key combination.
	//	Without this, a registered combination would be swallowed by the system and never reach the editor.
	void setSuspended(bool suspended);
	//	Release all hotkeys right now.
	void clear(void);

	bool isRegistered(Action action) const;

signals:
	void triggered(lmHotkeyHandler::Action action);
	//	A hotkey could not be registered, usually because another application owns it.
	void registrationFailed(lmHotkeyHandler::Action action, const QKeySequence& sequence, const QString& reason);

protected:
	bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
	void refresh(void);
	void unregisterAll(void);
	bool registerNative(Action action, const QKeySequence& sequence, QString* pReason);
	void unregisterNative(Action action);

	bool m_enabled = false;
	bool m_suspended = false;
	std::array<QKeySequence, HK_Max> m_sequences;
	std::array<bool, HK_Max> m_registered{};
	std::array<bool, HK_Max> m_failureReported{};
};

#endif // HOTKEYHANDLER_H
