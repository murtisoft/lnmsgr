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


#ifndef AFORMSETTINGS_H
#define AFORMSETTINGS_H

#include <QMediaDevices>
#include <QAudioSource>
#include <QTimer>
#include <QAudioDevice>
#include <QCameraDevice>
#include <QCamera>
#include <QScreenCapture>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QVideoSink>
#include <QVideoFrame>

#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QDialog>
#include <QListWidget>
#include <QFileDialog>
#include <QDesktopServices>
#include "ui_aformsettings.h"
#include "definitionssettings.h"
#include "widgetchatlog.h"
#include "hotkeyhandler.h"
#include <QKeySequenceEdit>


class lmFormSettings : public QDialog {
	Q_OBJECT

public:
	lmFormSettings(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
	~lmFormSettings(void);

	void init(void);
	void settingsChanged(void);
    void changeColorScheme(int index);
    void done(int result) override;
    bool isMicTesting(void) const { return micTesting; }
    //  The push to talk key went down / up while the microphone test is running. The bar flashes green meanwhile,
    //  like the real thing would transmit. Returns false when there is no push to talk test going on.
    bool testPushToTalk(bool down);

signals:
	void historyCleared(void);
	void fileHistoryCleared(void);
	//	true while one of the hotkey editors has focus. Global hotkeys must be released meanwhile, or the
	//	system swallows the key combination before the editor gets to see it.
	void hotkeyCaptureChanged(bool capturing);
	//	true while the microphone test is running. During a call the call microphone is muted meanwhile.
	void micTestChanged(bool testing);

protected:
	void changeEvent(QEvent* pEvent);
    void showEvent(QShowEvent* pEvent) override;
    void timerEvent(QTimerEvent *event);

private slots:
	void lvCategories_currentRowChanged(int currentRow);
	void btnOk_clicked(void);
	void chkMessageTime_toggled(bool checked);
	void chkAllowLinks_toggled(bool checked);
	void rdbSysHistoryPath_toggled(bool checked);
	void btnHistoryPath_clicked(void);
	void btnFilePath_clicked(void);
	void btnClearHistory_clicked(void);
	void btnClearFileHistory_clicked(void);
	void chkSound_toggled(bool checked);
	void chkAutoShowFile_toggled(bool checked);
	void btnViewFiles_clicked(void);
	void btnReset_clicked(void);
	void cboTheme_currentIndexChanged(int index);
	void lvBroadcasts_currentRowChanged(int index);
	void txtBroadcast_textEdited(const QString& text);
	void btnAddBroadcast_clicked(void);
	void btnDeleteBroadcast_clicked(void);
	void lvSounds_currentRowChanged(int index);
	void btnPlaySound_clicked(void);
	void btnSoundPath_clicked(void);
	void btnResetSounds_clicked(void);
	void chkGlobalHotkeys_toggled(bool checked);
	void cboMicDevice_activated(int index);
	void cboCamDevice_activated(int index);
	void btnTestMic_clicked(void);
	void onMicTestReadyRead(void);
	void onMicTestStateChanged(QAudio::State state);
	void application_focusChanged(QWidget* old, QWidget* now);


private:
    void setPageHeaderStyle(QLabel* pLabel);
	void setUIText(void);
	void loadSettings(void);
	void saveSettings(void);
    void insertStatusMessageinLog();
    void validateHotkeys(void);
    void updateActivationUi(void);
    void populateMicDevices(void);
    QAudioDevice chosenMicDevice(void) const;
    void setMicTesting(bool on);
    bool openMicTestSource(void);
    void closeMicTestSource(void);
    void populateCamDevices(void);
    void updateCamPreview(void);
    void stopCamPreview(void);

	Ui::SettingsDialog ui;
	lmSettings* pSettings;
	int fontSize;
	QIntValidator* pPortValidator;
	QRegularExpression ipRegExp;
    QRegularExpressionValidator* pIpValidator;
	lmChatLog* pMessageLog;
    int statusTimerId;
    int statusNow;
    QKeySequenceEdit* hotkeyEdits[lmHotkeyHandler::HK_Max];
    bool hotkeyCapturing = false;
    QMediaDevices* pMediaDevices = nullptr;
    QByteArray micDeviceId;     // QAudioDevice::id() of the chosen microphone, empty = default
    QString micDeviceName;      // its last known name, so it can be shown while unplugged
    QAudioSource* pMicTestSource = nullptr;
    QIODevice* pMicTestInput = nullptr;
    QAudioFormat micTestFormat;
    bool micTesting = false;
    QTimer* pPttTestTimer = nullptr;
    QString micTestIdleText;    // caption of the test button while it is not running
    QMediaCaptureSession* pCamSession = nullptr;
    QCamera* pCamera = nullptr;
    QScreenCapture* pScreenCapture = nullptr;
    QString camDeviceKey;       // see IDS_CAMDEVICE, empty = still image
    QString camDeviceName;      // its last known name, so it can be shown while unplugged
    QString micHotkeyIdleText;  // caption of the microphone hotkey while it is not push to talk
};

#endif // AFORMSETTINGS_H
