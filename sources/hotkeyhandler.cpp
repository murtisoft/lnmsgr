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

#include <QCoreApplication>
#include <QMetaObject>
#include "hotkeyhandler.h"

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

//Handling global hotkeys without having focus, is platform specific.
//Windows: RegisterHotKey() with no window handle. WM_HOTKEY is then posted to the GUI thread's message queue,
//         where we pick it up with a native event filter. No hidden window needed.
//Others:  not implemented yet. isSupported() says so, and the settings page disables itself.

namespace {

struct ActionInfo {
    const char* settingsKey;
    const char* defaultSequence;
};

const ActionInfo kActionInfo[lmHotkeyHandler::HK_Max] = {
    { IDS_HOTKEYMIC,     IDS_HOTKEYMIC_VAL     },
    { IDS_HOTKEYSPEAKER, IDS_HOTKEYSPEAKER_VAL },
    { IDS_HOTKEYCAMERA,  IDS_HOTKEYCAMERA_VAL  },
};

//  Stored in place of a key sequence when the user deliberately cleared a hotkey.
//  An empty value would be indistinguishable from "never set", which means "use the default".
const char kUnassigned[] = "none";

//  Hotkey ids only have to be unique within the thread. Must be in the 0x0000 - 0xBFFF range.
constexpr int kIdBase = 0x4C4D;

//  Keys that are safe to use without any modifier. Everything else would hijack normal typing system wide.
bool isStandaloneKey(Qt::Key key) {
    if(key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return true;
    switch(key) {
    case Qt::Key_Pause:
    case Qt::Key_Print:
    case Qt::Key_ScrollLock:
    case Qt::Key_MediaPlay:
    case Qt::Key_MediaTogglePlayPause:
    case Qt::Key_MediaStop:
    case Qt::Key_MediaPrevious:
    case Qt::Key_MediaNext:
        return true;
    default:
        return false;
    }
}

#ifdef Q_OS_WIN

//  Qt key -> Windows virtual key. Returns 0 if there is no usable mapping.
//  needsShift is set when the key is a shifted symbol on the current keyboard layout (e.g. '!').
UINT virtualKeyFor(Qt::Key key, bool& needsShift) {
    needsShift = false;

    //  Letters and digits: the virtual key code is the ASCII code of the uppercase letter / digit.
    if((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
        return UINT(key);
    if(key >= Qt::Key_F1 && key <= Qt::Key_F24)
        return VK_F1 + UINT(key - Qt::Key_F1);

    switch(key) {
    case Qt::Key_Space:         return VK_SPACE;
    case Qt::Key_Return:
    case Qt::Key_Enter:         return VK_RETURN;
    case Qt::Key_Tab:           return VK_TAB;
    case Qt::Key_Backspace:     return VK_BACK;
    case Qt::Key_Escape:        return VK_ESCAPE;
    case Qt::Key_Insert:        return VK_INSERT;
    case Qt::Key_Delete:        return VK_DELETE;
    case Qt::Key_Home:          return VK_HOME;
    case Qt::Key_End:           return VK_END;
    case Qt::Key_PageUp:        return VK_PRIOR;
    case Qt::Key_PageDown:      return VK_NEXT;
    case Qt::Key_Left:          return VK_LEFT;
    case Qt::Key_Up:            return VK_UP;
    case Qt::Key_Right:         return VK_RIGHT;
    case Qt::Key_Down:          return VK_DOWN;
    case Qt::Key_Pause:         return VK_PAUSE;
    case Qt::Key_Print:         return VK_SNAPSHOT;
    case Qt::Key_ScrollLock:    return VK_SCROLL;
    case Qt::Key_MediaPlay:
    case Qt::Key_MediaTogglePlayPause: return VK_MEDIA_PLAY_PAUSE;
    case Qt::Key_MediaStop:     return VK_MEDIA_STOP;
    case Qt::Key_MediaPrevious: return VK_MEDIA_PREV_TRACK;
    case Qt::Key_MediaNext:     return VK_MEDIA_NEXT_TRACK;
    default:
        break;
    }

    //  Punctuation and Latin-1 letters: Qt uses the character itself as key code. Ask the active keyboard
    //  layout which physical key produces it, so e.g. a comma ends up on the right key on every layout.
    const bool printableAscii = key > Qt::Key_Space && key <= Qt::Key_AsciiTilde;
    const bool latin1 = key >= 0xA0 && key <= 0xFF;
    if(printableAscii || latin1) {
        const SHORT scan = VkKeyScanW(WCHAR(QChar::toLower(char32_t(key))));
        if(scan == -1)
            return 0;
        needsShift = ((scan >> 8) & 1) != 0;
        return UINT(scan & 0xFF);
    }
    return 0;
}

bool toNative(const QKeySequence& sequence, UINT& mods, UINT& vk) {
    if(sequence.count() != 1)
        return false;
    const QKeyCombination combo = sequence[0];
    bool needsShift = false;
    vk = virtualKeyFor(combo.key(), needsShift);
    if(vk == 0)
        return false;

    const Qt::KeyboardModifiers m = combo.keyboardModifiers();
    mods = 0;
    if(m.testFlag(Qt::ControlModifier)) mods |= MOD_CONTROL;
    if(m.testFlag(Qt::AltModifier))     mods |= MOD_ALT;
    if(m.testFlag(Qt::MetaModifier))    mods |= MOD_WIN;
    if(m.testFlag(Qt::ShiftModifier) || needsShift) mods |= MOD_SHIFT;
    return true;
}

bool isKeySupported(Qt::Key key) {
    bool needsShift = false;
    return virtualKeyFor(key, needsShift) != 0;
}

#else

bool isKeySupported(Qt::Key) {
    return false;
}

#endif

}   // namespace

lmHotkeyHandler::lmHotkeyHandler(QObject* parent) : QObject(parent) {
    m_releaseTimer = new QTimer(this);
    m_releaseTimer->setInterval(15);
    connect(m_releaseTimer, &QTimer::timeout, this, &lmHotkeyHandler::pollHeld);
    if(QCoreApplication::instance())
        QCoreApplication::instance()->installNativeEventFilter(this);
}

lmHotkeyHandler::~lmHotkeyHandler(void) {
    blockSignals(true);     // whoever listens may already be half destroyed
    unregisterAll();
    if(QCoreApplication::instance())
        QCoreApplication::instance()->removeNativeEventFilter(this);
}

bool lmHotkeyHandler::isSupported(void) {
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QString lmHotkeyHandler::actionName(Action action) {
    switch(action) {
    case HK_Mic:        return tr("Microphone");
    case HK_Speaker:    return tr("Speaker");
    case HK_Camera:     return tr("Camera");
    default:            return QString();
    }
}

QKeySequence lmHotkeyHandler::defaultSequence(Action action) {
    return QKeySequence::fromString(QLatin1String(kActionInfo[action].defaultSequence), QKeySequence::PortableText);
}

QString lmHotkeyHandler::validate(const QKeySequence& sequence) {
    if(sequence.isEmpty())
        return QString();
    if(sequence.count() != 1)
        return tr("Use a single key combination.");

    const QKeyCombination combo = sequence[0];
    const Qt::KeyboardModifiers mods = combo.keyboardModifiers()
        & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);

    if(!isKeySupported(combo.key()))
        return tr("This key cannot be used as a global hotkey.");
    //  Shift alone doesn't count, Shift+A would block capital letters in every application.
    if(!(mods & ~Qt::KeyboardModifiers(Qt::ShiftModifier)) && !isStandaloneKey(combo.key()))
        return tr("Combine the key with Ctrl, Alt or Win.");
    return QString();
}

QKeySequence lmHotkeyHandler::loadSequence(const QSettings* pSettings, Action action) {
    const ActionInfo& info = kActionInfo[action];
    const QString text = pSettings->value(info.settingsKey, QLatin1String(info.defaultSequence)).toString().trimmed();
    if(text.compare(QLatin1String(kUnassigned), Qt::CaseInsensitive) == 0)
        return QKeySequence();

    QKeySequence sequence = QKeySequence::fromString(text, QKeySequence::PortableText);
    if(sequence.isEmpty())
        return defaultSequence(action);     // empty or unreadable, fall back
    if(sequence.count() > 1)
        sequence = QKeySequence(sequence[0]);
    return sequence;
}

void lmHotkeyHandler::saveSequence(lmSettingsBase* pSettings, Action action, const QKeySequence& sequence) {
    const ActionInfo& info = kActionInfo[action];
    const QString text = sequence.isEmpty() ? QString::fromLatin1(kUnassigned)
                                            : sequence.toString(QKeySequence::PortableText);
    pSettings->setValue(info.settingsKey, text, QString::fromLatin1(info.defaultSequence));
}

void lmHotkeyHandler::loadSettings(const QSettings* pSettings) {
    const bool enabled = pSettings->value(IDS_GLOBALHOTKEYS, IDS_GLOBALHOTKEYS_VAL).toBool();
    std::array<QKeySequence, HK_Max> sequences;
    for(int index = 0; index < HK_Max; index++)
        sequences[index] = loadSequence(pSettings, Action(index));

    //  Settings get reloaded for all sorts of reasons. Don't touch the registrations (or nag about
    //  conflicts again) unless something we care about actually changed.
    if(enabled == m_enabled && sequences == m_sequences)
        return;

    m_enabled = enabled;
    m_sequences = sequences;
    m_failureReported.fill(false);
    refresh();
}

void lmHotkeyHandler::setSuspended(bool suspended) {
    if(suspended == m_suspended)
        return;
    m_suspended = suspended;
    refresh();
}

void lmHotkeyHandler::setActive(bool active) {
    if(active == m_active)
        return;
    m_active = active;
    refresh();
}

void lmHotkeyHandler::clear(void) {
    unregisterAll();
    m_enabled = false;
    m_active = false;
    m_sequences = {};
    m_failureReported.fill(false);
}

bool lmHotkeyHandler::isRegistered(Action action) const {
    return m_registered[action];
}

void lmHotkeyHandler::refresh(void) {
    unregisterAll();
    if(!m_enabled || !m_active || m_suspended || !isSupported())
        return;

    for(int index = 0; index < HK_Max; index++) {
        const Action action = Action(index);
        const QKeySequence& sequence = m_sequences[index];
        if(sequence.isEmpty())
            continue;

        QString reason = validate(sequence);
        if(reason.isEmpty()) {
            //  The system would report these as "owned by another application", which would be misleading.
            for(int other = 0; other < index; other++) {
                if(m_registered[other] && m_sequences[other] == sequence) {
                    reason = tr("Same combination as the %1 hotkey.").arg(actionName(Action(other)));
                    break;
                }
            }
        }
        const bool ok = reason.isEmpty() && registerNative(action, sequence, &reason);
        m_registered[index] = ok;

        if(!ok && !m_failureReported[index]) {
            m_failureReported[index] = true;
            emit registrationFailed(action, sequence, reason);
        }
    }
}

void lmHotkeyHandler::beginHold(Action action) {
    m_held[action] = true;
    if(!m_releaseTimer->isActive())
        m_releaseTimer->start();
}

void lmHotkeyHandler::pollHeld(void) {
    bool any = false;
    for(int index = 0; index < HK_Max; index++) {
        if(!m_held[index])
            continue;
        if(isHeld(Action(index))) {
            any = true;
        } else {
            m_held[index] = false;
            emit released(Action(index));
        }
    }
    if(!any)
        m_releaseTimer->stop();
}

void lmHotkeyHandler::unregisterAll(void) {
    //  A key that is still down will never be seen coming up once it is unregistered.
    m_releaseTimer->stop();
    for(int index = 0; index < HK_Max; index++) {
        if(m_held[index]) {
            m_held[index] = false;
            emit released(Action(index));
        }
    }
    for(int index = 0; index < HK_Max; index++) {
        if(m_registered[index])
            unregisterNative(Action(index));
        m_registered[index] = false;
    }
}

#ifdef Q_OS_WIN

bool lmHotkeyHandler::registerNative(Action action, const QKeySequence& sequence, QString* pReason) {
    UINT mods = 0;
    UINT vk = 0;
    if(!toNative(sequence, mods, vk)) {
        *pReason = tr("This key cannot be used as a global hotkey.");
        return false;
    }
    //  MOD_NOREPEAT: holding the keys down toggles once, not 30 times per second.
    if(!RegisterHotKey(nullptr, kIdBase + int(action), mods | MOD_NOREPEAT, vk)) {
        const DWORD error = GetLastError();
        *pReason = (error == ERROR_HOTKEY_ALREADY_REGISTERED)
            ? tr("Already in use by another application.")
            : tr("Windows refused the hotkey (error %1).").arg(error);
        return false;
    }
    m_nativeMods[action] = mods;
    m_nativeKey[action] = vk;
    return true;
}

//  The system only says when a hotkey was pressed. To find out when it is let go, look at the keyboard
//  state every few milliseconds for as long as it is held. Cheap, and only runs while a hotkey is down.
bool lmHotkeyHandler::isHeld(Action action) const {
    auto down = [](int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; };
    const UINT mods = m_nativeMods[action];
    if(!down(int(m_nativeKey[action])))
        return false;
    if((mods & MOD_CONTROL) && !down(VK_CONTROL))
        return false;
    if((mods & MOD_ALT) && !down(VK_MENU))
        return false;
    if((mods & MOD_SHIFT) && !down(VK_SHIFT))
        return false;
    if((mods & MOD_WIN) && !down(VK_LWIN) && !down(VK_RWIN))
        return false;
    return true;
}

void lmHotkeyHandler::unregisterNative(Action action) {
    UnregisterHotKey(nullptr, kIdBase + int(action));
}

bool lmHotkeyHandler::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    //  Thread messages (no window handle), which is what we get from RegisterHotKey(nullptr, ...),
    //  come through the dispatcher. Window messages come as generic. Accept both to be safe.
    if(eventType != "windows_dispatcher_MSG" && eventType != "windows_generic_MSG")
        return false;

    const MSG* msg = static_cast<const MSG*>(message);
    if(msg->message != WM_HOTKEY)
        return false;

    const int index = int(msg->wParam) - kIdBase;
    if(index < 0 || index >= HK_Max || !m_registered[index])
        return false;

    //  Queue it, so whatever the slot does (sounds, UI) doesn't run inside the native message pump.
    const Action action = Action(index);
    QMetaObject::invokeMethod(this, [this, action]() {
        if(!m_registered[action])
            return;
        emit triggered(action);
        beginHold(action);
    }, Qt::QueuedConnection);
    if(result)
        *result = 0;
    return true;
}

#else

bool lmHotkeyHandler::registerNative(Action, const QKeySequence&, QString* pReason) {
    *pReason = tr("Global hotkeys are not supported on this platform.");
    return false;
}

void lmHotkeyHandler::unregisterNative(Action) {
}

bool lmHotkeyHandler::isHeld(Action) const {
    return false;
}

bool lmHotkeyHandler::nativeEventFilter(const QByteArray&, void*, qintptr*) {
    return false;
}

#endif
