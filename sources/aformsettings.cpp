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


#include <QUrl>
#include <QSystemTrayIcon>
#include <QLocale>
#include <QMessageBox>
#include <QToolTip>
#include <QTimer>
#include <qstylehints.h>
#include "aformsettings.h"
#include "zdebuglog.h"
#include "soundplayer.h"
#include "historydatabase.h"
#include "translationloader.h"
#include "translatabletext.h"
#include "definitionsdir.h"
#include "audiolevel.h"
#include "videocapture.h"
#include <QScreen>
#include <QGuiApplication>

lmFormSettings::lmFormSettings(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags) {
    ui.setupUi(this);
    //	remove the help button from window button group
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //	Destroy the window when it closes
    setAttribute(Qt::WA_DeleteOnClose, true);

    pMessageLog = new lmChatLog(ui.fraMessageLog);
    ui.logLayout->addWidget(pMessageLog);

    statusTimerId = 0;
    statusNow = 0;

    connect(ui.lvCategories, SIGNAL(currentRowChanged(int)), this, SLOT(lvCategories_currentRowChanged(int)));
    connect(ui.btnOK, SIGNAL(clicked()), this, SLOT(btnOk_clicked()));
    connect(ui.chkMessageTime, SIGNAL(toggled(bool)), this, SLOT(chkMessageTime_toggled(bool)));
    connect(ui.chkAllowLinks, SIGNAL(toggled(bool)), this, SLOT(chkAllowLinks_toggled(bool)));
    connect(ui.rdbSysHistoryPath, SIGNAL(toggled(bool)), this, SLOT(rdbSysHistoryPath_toggled(bool)));
    connect(ui.btnHistoryPath, SIGNAL(clicked()), this, SLOT(btnHistoryPath_clicked()));
    connect(ui.btnFilePath, SIGNAL(clicked()), this, SLOT(btnFilePath_clicked()));
    connect(ui.btnClearHistory, SIGNAL(clicked()), this, SLOT(btnClearHistory_clicked()));
    connect(ui.btnClearFileHistory, SIGNAL(clicked()), this, SLOT(btnClearFileHistory_clicked()));
    connect(ui.btnViewFiles, SIGNAL(clicked()), this, SLOT(btnViewFiles_clicked()));
    connect(ui.chkSound, SIGNAL(toggled(bool)), this, SLOT(chkSound_toggled(bool)));
    connect(ui.chkAutoShowFile, SIGNAL(toggled(bool)), this, SLOT(chkAutoShowFile_toggled(bool)));
    connect(ui.btnReset, SIGNAL(clicked()), this, SLOT(btnReset_clicked()));
    connect(ui.cboTheme, SIGNAL(currentIndexChanged(int)), this, SLOT(cboTheme_currentIndexChanged(int)));
    connect(ui.lvBroadcasts, SIGNAL(currentRowChanged(int)), this, SLOT(lvBroadcasts_currentRowChanged(int)));
    connect(ui.txtBroadcast, SIGNAL(textEdited(QString)), this, SLOT(txtBroadcast_textEdited(QString)));
    connect(ui.txtBroadcast, SIGNAL(returnPressed()), this, SLOT(btnAddBroadcast_clicked()));
    connect(ui.btnAddBroadcast, SIGNAL(clicked()), this, SLOT(btnAddBroadcast_clicked()));
    connect(ui.btnDeleteBroadcast, SIGNAL(clicked()), this, SLOT(btnDeleteBroadcast_clicked()));
    connect(ui.lvSounds, SIGNAL(currentRowChanged(int)), this, SLOT(lvSounds_currentRowChanged(int)));
    connect(ui.btnPlaySound, SIGNAL(clicked()), this, SLOT(btnPlaySound_clicked()));
    connect(ui.btnSoundPath, SIGNAL(clicked()), this, SLOT(btnSoundPath_clicked()));
    connect(ui.btnResetSounds, SIGNAL(clicked()), this, SLOT(btnResetSounds_clicked()));

    hotkeyEdits[lmHotkeyHandler::HK_Mic] = ui.keyMicHotkey;
    hotkeyEdits[lmHotkeyHandler::HK_Speaker] = ui.keySpeakerHotkey;
    hotkeyEdits[lmHotkeyHandler::HK_Camera] = ui.keyCameraHotkey;
    QPushButton* hotkeyDefaults[lmHotkeyHandler::HK_Max] = {
        ui.btnMicHotkeyDefault, ui.btnSpeakerHotkeyDefault, ui.btnCameraHotkeyDefault
    };
    for(int index = 0; index < lmHotkeyHandler::HK_Max; index++) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        hotkeyEdits[index]->setMaximumSequenceLength(1);    // one combination, not a chord
        hotkeyEdits[index]->setClearButtonEnabled(true);    // clearing leaves the action without a hotkey
#endif
        connect(hotkeyEdits[index], &QKeySequenceEdit::keySequenceChanged, this, &lmFormSettings::validateHotkeys);
        connect(hotkeyDefaults[index], &QPushButton::clicked, this, [this, index]() {
            hotkeyEdits[index]->setKeySequence(lmHotkeyHandler::defaultSequence(lmHotkeyHandler::Action(index)));
        });
    }
    connect(ui.chkGlobalHotkeys, &QCheckBox::toggled, this, &lmFormSettings::chkGlobalHotkeys_toggled);
    connect(ui.cboMicDevice, &QComboBox::activated, this, &lmFormSettings::cboMicDevice_activated);
    connect(ui.cboCamDevice, &QComboBox::activated, this, &lmFormSettings::cboCamDevice_activated);
    connect(ui.btnTestMic, &QPushButton::clicked, this, &lmFormSettings::btnTestMic_clicked);
    connect(ui.cboMicActivation, &QComboBox::currentIndexChanged, this, &lmFormSettings::updateActivationUi);
    //  Mimics the tail that push to talk transmits after the key is let go
    pPttTestTimer = new QTimer(this);
    pPttTestTimer->setSingleShot(true);
    pPttTestTimer->setInterval(MIC_PTT_RELEASE_MS);
    connect(pPttTestTimer, &QTimer::timeout, this, [this]() { ui.sldMicThreshold->setTriggered(false); });
    ui.lblMicWarning->setStyleSheet("color: #d9534f;");
    ui.lblMicWarning->hide();
    pMediaDevices = new QMediaDevices(this);
    pCamSession = new QMediaCaptureSession(this);
    pCamSession->setVideoOutput(ui.vidCamPreview);
    connect(pMediaDevices, &QMediaDevices::audioInputsChanged, this, &lmFormSettings::populateMicDevices);
    connect(pMediaDevices, &QMediaDevices::videoInputsChanged, this, &lmFormSettings::populateCamDevices);
    connect(qApp, &QGuiApplication::screenAdded, this, &lmFormSettings::populateCamDevices);
    connect(qApp, &QGuiApplication::screenRemoved, this, &lmFormSettings::populateCamDevices);
    connect(qApp, &QApplication::focusChanged, this, &lmFormSettings::application_focusChanged);
    ui.lblHotkeyWarning->setStyleSheet("color: #d9534f;");
    ui.lblHotkeyWarning->hide();
}

lmFormSettings::~lmFormSettings(void) {
    closeMicTestSource();
}

//  Every way out of the dialog ends up here, make sure the microphone is released.
void lmFormSettings::done(int result) {
    setMicTesting(false);
    stopCamPreview();
    QDialog::done(result);
}

void lmFormSettings::init(void) {
    QMap<QString, QString> languages;
    //	Loop through available languages and add them to a map. This ensures that
    //	the languages are sorted alphabetically. After that add the sorted items
    //	to the combo box.
    for(int index = 0; index < Application::availableLanguages().count(); index++) {
        QString langCode = Application::availableLanguages().value(index);
        QLocale locale(langCode);
        QString language = QLocale::languageToString(locale.language());
        languages.insert(language, langCode);
    }
    for (auto it = languages.cbegin(); it != languages.cend(); ++it)
        ui.cboLanguage->addItem(it.key(), it.value());

    for(int index = 0; index < FS_COUNT; index++)
        ui.cboFontSize->addItem(lmStrings::fontSize()[index], index);

    for(int index = 0; index < SE_Max; index++) {
        QListWidgetItem* pListItem = new QListWidgetItem(ui.lvSounds);
        pListItem->setText(lmStrings::soundDesc()[index]);
        pListItem->setData(Qt::UserRole, soundFile[index]);
        pListItem->setCheckState(IDS_SOUNDEVENT_VAL);
    }

    for(int index = 0; index < ULV_Max; index++)
        ui.cboUserListView->addItem(lmStrings::userListView()[index], index);

    fontSize = 0;
    ui.lvCategories->setCurrentRow(0);

    setWindowIcon(QIcon(IDR_APPICON));

    ui.lvCategories->setIconSize(QSize(32, 32));
    ui.lvCategories->item(0)->setIcon(Helper::renderEmoji(Icons::GeneralSet,32));
    ui.lvCategories->item(1)->setIcon(Helper::renderEmoji(Icons::AccountSet,32));
    ui.lvCategories->item(2)->setIcon(Helper::renderEmoji(Icons::MessageSet,32));
    ui.lvCategories->item(3)->setIcon(Helper::renderEmoji(Icons::VoiceSet,32));
    ui.lvCategories->item(4)->setIcon(Helper::renderEmoji(Icons::HistorySet,32));
    ui.lvCategories->item(5)->setIcon(Helper::renderEmoji(Icons::AlertSet,32));
    ui.lvCategories->item(6)->setIcon(Helper::renderEmoji(Icons::NetworkSet,32));
    ui.lvCategories->item(7)->setIcon(Helper::renderEmoji(Icons::TransferSet,32));
    ui.lvCategories->item(8)->setIcon(Helper::renderEmoji(Icons::ThemeSet,32));
    ui.lvCategories->item(9)->setIcon(Helper::renderEmoji(Icons::HotkeySet,32));

    setPageHeaderStyle(ui.lblGeneralPage);
    setPageHeaderStyle(ui.lblAccountPage);
    setPageHeaderStyle(ui.lblMessagesPage);
    setPageHeaderStyle(ui.lblVoiceVideoPage);
    setPageHeaderStyle(ui.lblHistoryPage);
    setPageHeaderStyle(ui.lblAlertsPage);
    setPageHeaderStyle(ui.lblNetworkPage);
    setPageHeaderStyle(ui.lblFileTransferPage);
    setPageHeaderStyle(ui.lblThemePage);
    setPageHeaderStyle(ui.lblHotkeysPage);

    ui.btnPlaySound->setIcon(QIcon(Helper::renderEmoji(Icons::Play,16)));

    pPortValidator = new QIntValidator(1, 65535, this);
    ui.txtUDPPort->setValidator(pPortValidator);
    ui.txtTCPPort->setValidator(pPortValidator);

    QRegularExpression ipRegExp(R"(\b(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\b)");
    pIpValidator = new QRegularExpressionValidator(ipRegExp, this);
    ui.txtMulticast->setValidator(pIpValidator);

    pMessageLog->setAutoScroll(false);

    pSettings = new lmSettings();
    setUIText();
    loadSettings();
}

void lmFormSettings::settingsChanged(void) {
    loadSettings();
}

void lmFormSettings::changeEvent(QEvent* pEvent) {
    switch(pEvent->type()) {
    case QEvent::LanguageChange:
        setUIText();
        break;
    default:
        break;
    }

    QDialog::changeEvent(pEvent);
}

//  The avatar may have been changed since the last time the dialog was open.
void lmFormSettings::showEvent(QShowEvent* pEvent) {
    QDialog::showEvent(pEvent);
    updateCamPreview();
}

void lmFormSettings::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == statusTimerId) {
        MessageXml msg;
        msg.addData(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
        msg.addData(XN_FONT, QFont().toString());
        QString userId = "Jack";
        QString userName = "Jack";
        msg.removeData(XN_MESSAGE);
        switch (statusNow++) {
        case 0:
            msg.addData(XN_CHATSTATE, "");
            break;
        case 1:
            msg.addData(XN_CHATSTATE, "composing");
            break;
        default:
            msg.addData(XN_CHATSTATE, "paused");
            statusNow = 0;
            break;
        }
        pMessageLog->appendMessageLog(MT_ChatState, &userId, &userName, &msg, true);
    }
}

void lmFormSettings::lvCategories_currentRowChanged(int currentRow) {
    ui.stackedWidget->setCurrentIndex(currentRow);
    //  Only listen while the user is looking at the meter.
    if(micTesting && ui.stackedWidget->currentWidget() != ui.pageVoiceVideo)
        setMicTesting(false);
    updateCamPreview();
}

void lmFormSettings::btnOk_clicked(void) {
    saveSettings();
}

void lmFormSettings::chkMessageTime_toggled(bool checked) {
    Q_UNUSED(checked);

    ui.chkMessageDate->setEnabled(ui.chkMessageTime->isChecked());
}

void lmFormSettings::chkAllowLinks_toggled(bool checked) {
    Q_UNUSED(checked);

    ui.chkPathToLink->setEnabled(ui.chkAllowLinks->isChecked());
}

void lmFormSettings::rdbSysHistoryPath_toggled(bool checked) {
    ui.txtHistoryPath->setEnabled(!checked);
    ui.btnHistoryPath->setEnabled(!checked);

    if(!checked) {
        ui.txtHistoryPath->setText(History::historyFile());
    } else {
        QString path = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" HC_FILENAME);
        ui.txtHistoryPath->setText(path);
    }
}

void lmFormSettings::btnHistoryPath_clicked(void) {
    QString historyPath = QFileDialog::getSaveFileName(this, tr("Save History"),
                                                       ui.txtHistoryPath->text(), "Messenger DB (*.db)");
    if(!historyPath.isEmpty())
        ui.txtHistoryPath->setText(historyPath);
}

void lmFormSettings::btnFilePath_clicked(void) {
    QString filePath = QFileDialog::getExistingDirectory(this, tr("Select folder"),
                                                         ui.txtFilePath->text(), QFileDialog::ShowDirsOnly);
    if(!filePath.isEmpty())
        ui.txtFilePath->setText(filePath);
}

void lmFormSettings::btnClearHistory_clicked(void) {
    QFile::remove(History::historyFile());
    emit historyCleared();
}

void lmFormSettings::btnClearFileHistory_clicked(void) {
    QFile::remove(DefinitionsDir::transferHistory());
    emit fileHistoryCleared();
}

void lmFormSettings::chkSound_toggled(bool checked) {
    ui.lvSounds->setEnabled(checked);
}

void lmFormSettings::chkAutoShowFile_toggled(bool checked) {
    ui.rdbFileTop->setEnabled(checked);
    ui.rdbFileBottom->setEnabled(checked);
}

void lmFormSettings::btnViewFiles_clicked(void) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(ui.txtFilePath->text()));
}

void lmFormSettings::btnReset_clicked(void) {
    QString message = tr("Are you sure you want to reset your %1 preferences?");
    if(QMessageBox::question(this, tr("Reset Preferences"), message.arg(lmStrings::appName()), QMessageBox::Yes, QMessageBox::No)
        == QMessageBox::Yes) {
        QFile::remove(pSettings->fileName());
        pSettings->sync();
        emit settingsReset();
        accept();
    }
}

void lmFormSettings::cboTheme_currentIndexChanged(int index) {

    pMessageLog->fontSizeVal = FS_SMALL;
    pMessageLog->localId = "Myself";
    pMessageLog->peerId = "Jack";
    pMessageLog->messageTime = true;
    pMessageLog->initMessageLog();

    MessageXml msg;
    msg.addData(XN_TIME, QString::number(QDateTime::currentMSecsSinceEpoch()));
    msg.addData(XN_FONT, QFont().toString());

    QString userId = "Jack";
    QString userName = "Jack";

    msg.addData(XN_MESSAGE, "Hello, this is an incoming message.");
    pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

    msg.removeData(XN_MESSAGE);
    msg.addData(XN_MESSAGE, "Hello, this is a consecutive incoming message.");
    pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

    msg.removeData(XN_MESSAGE);
    msg.addData(XN_BROADCAST, "This is a broadcast message!");
    pMessageLog->appendMessageLog(MT_Broadcast, &userId, &userName, &msg, true);

    userId = "Myself";
    userName = "Myself";

    msg.removeData(XN_BROADCAST);
    msg.addData(XN_MESSAGE, "Hi, this is an outgoing message.");
    pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

    msg.removeData(XN_MESSAGE);
    msg.addData(XN_MESSAGE, "Hi, this is a consecutive outgoing message.");
    pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

    userId = "Jack";
    userName = "Jack";

    pMessageLog->autoFile = false;
    MessageXml fileMsg;
    auto fileId = Helper::getUuid();
    fileMsg.addData(XN_FILEPATH, "C:\\unreal file path\\file name.txt");
    fileMsg.addData(XN_FILEID, fileId);
    fileMsg.addData(XN_MODE, FileModeNames[FileMode::FM_Receive]);
    fileMsg.addData(XN_FILENAME, "test_file.txt");
    fileMsg.addData(XN_FILESIZE, "800000");
    fileMsg.addData(XN_FILEOP, FileOpNames[FileOp::FO_Request]);
    bool bReload = true;
#ifndef QT_NO_DEBUG
    bReload = false;
#endif
    pMessageLog->appendMessageLog(MT_File, &userId, &userName, &fileMsg, bReload);

    msg.removeData(XN_MESSAGE);
    msg.addData(XN_MESSAGE, "This is another incoming message.");
    pMessageLog->appendMessageLog(MT_Message, &userId, &userName, &msg, true);

    killTimer(statusTimerId);
    statusTimerId = startTimer(3000);
}

void lmFormSettings::lvBroadcasts_currentRowChanged(int index) {
    ui.btnDeleteBroadcast->setEnabled(!(index < 0));
}

void lmFormSettings::txtBroadcast_textEdited(const QString& text) {
    ui.btnAddBroadcast->setEnabled(ipRegExp.match(text).hasMatch());
}

void lmFormSettings::btnAddBroadcast_clicked(void) {
    QString address = ui.txtBroadcast->text();
    //	Do not add if not a valid ip address
    if (!ipRegExp.match(address).hasMatch())
        return;

    //	Check if the same address is already present in the list
    for(int index = 0; index < ui.lvBroadcasts->count(); index++) {
        QString text = ui.lvBroadcasts->item(index)->text();
        if(text.compare(address) == 0)
            return;
    }

    QListWidgetItem* item = new QListWidgetItem(ui.lvBroadcasts);
    item->setText(address);

    ui.txtBroadcast->clear();
    ui.btnAddBroadcast->setEnabled(false);
    ui.txtBroadcast->setFocus();
}

void lmFormSettings::btnDeleteBroadcast_clicked(void) {
    if(ui.lvBroadcasts->currentRow() < 0)
        return;

    QListWidgetItem* item = ui.lvBroadcasts->takeItem(ui.lvBroadcasts->currentRow());
    delete item;
}

void lmFormSettings::lvSounds_currentRowChanged(int index) {
    ui.btnPlaySound->setEnabled(!(index < 0));
    ui.btnSoundPath->setEnabled(!(index < 0));

    if(index < 0) {
        ui.txtSoundFile->clear();
        return;
    }

    QFileInfo fileInfo(ui.lvSounds->currentItem()->data(Qt::UserRole).toString());
    if(fileInfo.exists())
        ui.txtSoundFile->setText(fileInfo.fileName());
    else
        ui.txtSoundFile->setText(tr("<File Not Found>"));
}

void lmFormSettings::btnPlaySound_clicked(void) {
    if(ui.lvSounds->currentRow() < 0)
        return;

    lmSoundPlayer::play(ui.lvSounds->currentItem()->data(Qt::UserRole).toString());
}

void lmFormSettings::btnSoundPath_clicked(void) {
    if(ui.lvSounds->currentRow() < 0)
        return;

    QString soundPath = QFileDialog::getOpenFileName(this, tr("Select sound"),
                                                     ui.lvSounds->currentItem()->data(Qt::UserRole).toString(), "Wave Files (*.wav)");
    if(!soundPath.isEmpty()) {
        ui.lvSounds->currentItem()->setData(Qt::UserRole, soundPath);
        lvSounds_currentRowChanged(ui.lvSounds->currentRow());
    }
}

void lmFormSettings::btnResetSounds_clicked(void) {
    for(int index = 0; index < SE_Max; index++) {
        QListWidgetItem* pListItem = ui.lvSounds->item(index);
        pListItem->setData(Qt::UserRole, soundFile[index]);
    }
    lvSounds_currentRowChanged(ui.lvSounds->currentRow());
}

void lmFormSettings::cboMicDevice_activated(int index) {
    micDeviceId = ui.cboMicDevice->itemData(index).toByteArray();
    micDeviceName = ui.cboMicDevice->itemData(index, Qt::UserRole + 1).toString();
    if(micTesting)
        setMicTesting(true);    // carry on with the new device
}

void lmFormSettings::btnTestMic_clicked(void) {
    setMicTesting(!micTesting);
}

//  The chosen microphone, or a null device when it is not connected. Unlike a call, the test does not
//  fall back to the default one: testing a different microphone than the one picked would only mislead.
QAudioDevice lmFormSettings::chosenMicDevice(void) const {
    if(micDeviceId.isEmpty())
        return QMediaDevices::defaultAudioInput();
    const auto devices = QMediaDevices::audioInputs();
    for(const QAudioDevice& device : devices)
        if(device.id() == micDeviceId)
            return device;
    return QAudioDevice();
}

bool lmFormSettings::openMicTestSource(void) {
    closeMicTestSource();

    const QAudioDevice device = chosenMicDevice();
    if(device.isNull())
        return false;

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if(!device.isFormatSupported(format))
        format = device.preferredFormat();

    pMicTestSource = new QAudioSource(device, format, this);
    micTestFormat = format;
    connect(pMicTestSource, &QAudioSource::stateChanged, this, &lmFormSettings::onMicTestStateChanged);
    pMicTestInput = pMicTestSource->start();
    if(!pMicTestInput) {
        closeMicTestSource();
        return false;
    }
    connect(pMicTestInput, &QIODevice::readyRead, this, &lmFormSettings::onMicTestReadyRead);
    return true;
}

void lmFormSettings::closeMicTestSource(void) {
    if(pMicTestSource) {
        pMicTestSource->disconnect(this);
        pMicTestSource->stop();
        delete pMicTestSource;
        pMicTestSource = nullptr;
    }
    pMicTestInput = nullptr;
}

//  Starts, restarts (on the current device) or stops the test. The test only runs when asked for, so the
//  system's "microphone in use" indicator is not on just because the dialog is open.
void lmFormSettings::setMicTesting(bool on) {
    if(on) {
        if(!openMicTestSource()) {
            if(!micTesting)     // the user pressed the button, tell them why nothing happens
                QToolTip::showText(ui.btnTestMic->mapToGlobal(QPoint(0, ui.btnTestMic->height())),
                                   tr("The microphone is not available."), ui.btnTestMic);
            on = false;
        }
    } else {
        closeMicTestSource();
    }

    if(!on) {
        ui.sldMicThreshold->resetLevel();
        pPttTestTimer->stop();
        ui.sldMicThreshold->setTriggered(false);
    }
    ui.btnTestMic->setText(on ? tr("Stop") : micTestIdleText);
    if(on != micTesting) {
        micTesting = on;
        emit micTestChanged(on);
    }
}

bool lmFormSettings::testPushToTalk(bool down) {
    if(!micTesting || ui.cboMicActivation->currentIndex() != MA_PushToTalk)
        return false;
    if(down) {
        pPttTestTimer->stop();
        ui.sldMicThreshold->setTriggered(true);
    } else {
        pPttTestTimer->start();
    }
    return true;
}

void lmFormSettings::onMicTestReadyRead(void) {
    if(!pMicTestInput)
        return;
    const QByteArray data = pMicTestInput->readAll();
    if(!data.isEmpty())
        ui.sldMicThreshold->setLevel(lmAudioLevel::rmsDbfs(data, micTestFormat.sampleFormat()));
}

//  The device stopped by itself, most likely unplugged.
void lmFormSettings::onMicTestStateChanged(QAudio::State state) {
    if(state == QAudio::StoppedState && pMicTestSource && pMicTestSource->error() != QAudio::NoError)
        QTimer::singleShot(0, this, [this]() { setMicTesting(false); });    // not from inside its own signal
}

//  Lists the microphones and selects the chosen one. Runs again whenever a device is plugged in or removed.
//  The chosen device stays in the list even when it is not connected, so that pressing OK doesn't
//  silently forget it. Calls use the default microphone until it is back.
void lmFormSettings::populateMicDevices(void) {
    const QSignalBlocker blocker(ui.cboMicDevice);
    ui.cboMicDevice->clear();
    ui.cboMicDevice->addItem(tr("Default Communication Device"), QByteArray());

    int selected = micDeviceId.isEmpty() ? 0 : -1;
    const auto devices = QMediaDevices::audioInputs();
    for(const QAudioDevice& device : devices) {
        ui.cboMicDevice->addItem(device.description(), device.id());
        const int row = ui.cboMicDevice->count() - 1;
        ui.cboMicDevice->setItemData(row, device.description(), Qt::UserRole + 1);
        if(selected < 0 && device.id() == micDeviceId) {
            selected = row;
            micDeviceName = device.description();   // names can change, keep it current
        }
    }
    if(selected < 0) {
        const QString name = micDeviceName.isEmpty() ? tr("Unknown device") : micDeviceName;
        ui.cboMicDevice->addItem(tr("%1 (Not connected)").arg(name), micDeviceId);
        selected = ui.cboMicDevice->count() - 1;
        ui.cboMicDevice->setItemData(selected, name, Qt::UserRole + 1);
    }
    ui.cboMicDevice->setCurrentIndex(selected);

    if(micTesting)
        setMicTesting(true);    // the list changed: carries on, or stops if the microphone is gone
}

//  Same idea as the microphones: the chosen source stays in the list while it is not connected, so that
//  pressing OK doesn't silently forget it.
void lmFormSettings::populateCamDevices(void) {
    const QSignalBlocker blocker(ui.cboCamDevice);
    ui.cboCamDevice->clear();

    int selected = camDeviceKey.isEmpty() ? 0 : -1;
    auto addDevice = [&](const QString& text, const QString& key) {
        ui.cboCamDevice->addItem(text, key);
        const int row = ui.cboCamDevice->count() - 1;
        ui.cboCamDevice->setItemData(row, text, Qt::UserRole + 1);
        if(selected < 0 && key == camDeviceKey) {
            selected = row;
            camDeviceName = text;   // names can change, keep it current
        }
    };

    addDevice(tr("Still Image"), QString());
    const auto screens = QGuiApplication::screens();
    for(int index = 0; index < screens.size(); index++)
        addDevice(tr("Desktop %1").arg(index + 1), lmVideoCapture::screenKey(screens[index]));
    const auto cameras = QMediaDevices::videoInputs();
    for(const QCameraDevice& camera : cameras)
        addDevice(camera.description(), lmVideoCapture::cameraKey(camera));

    if(selected < 0) {
        const QString name = camDeviceName.isEmpty() ? tr("Unknown device") : camDeviceName;
        ui.cboCamDevice->addItem(tr("%1 (Not connected)").arg(name), camDeviceKey);
        selected = ui.cboCamDevice->count() - 1;
        ui.cboCamDevice->setItemData(selected, name, Qt::UserRole + 1);
    }
    ui.cboCamDevice->setCurrentIndex(selected);
    updateCamPreview();
}

void lmFormSettings::cboCamDevice_activated(int index) {
    camDeviceKey = ui.cboCamDevice->itemData(index).toString();
    camDeviceName = ui.cboCamDevice->itemData(index, Qt::UserRole + 1).toString();
    updateCamPreview();
}

//  Desktops and cameras are only opened while the Voice & Video page is showing, so the camera light is
//  not on for a hidden page. A source that is not connected leaves the preview black.
void lmFormSettings::updateCamPreview(void) {
    stopCamPreview();
    if(!isVisible() || ui.stackedWidget->currentWidget() != ui.pageVoiceVideo)
        return;

    if(QScreen* pScreen = lmVideoCapture::screenForKey(camDeviceKey)) {
        pScreenCapture = new QScreenCapture(this);
        pScreenCapture->setScreen(pScreen);
        pCamSession->setScreenCapture(pScreenCapture);
        pScreenCapture->start();
    } else if(const QCameraDevice device = lmVideoCapture::cameraForKey(camDeviceKey); !device.isNull()) {
        pCamera = new QCamera(device, this);
        pCamSession->setCamera(pCamera);
        pCamera->start();
    } else if(camDeviceKey.isEmpty()) {
        ui.vidCamPreview->showImage(lmVideoCapture::stillImage());
    }
}

void lmFormSettings::stopCamPreview(void) {
    if(pCamera) {
        pCamera->stop();
        pCamSession->setCamera(nullptr);
        delete pCamera;
        pCamera = nullptr;
    }
    if(pScreenCapture) {
        pScreenCapture->stop();
        pCamSession->setScreenCapture(nullptr);
        delete pScreenCapture;
        pScreenCapture = nullptr;
    }
    ui.vidCamPreview->videoSink()->setVideoFrame(QVideoFrame());
}

void lmFormSettings::chkGlobalHotkeys_toggled(bool checked) {
    ui.wdgCallHotkeys->setEnabled(checked);
    validateHotkeys();
}

void lmFormSettings::application_focusChanged(QWidget* old, QWidget* now) {
    Q_UNUSED(old);

    bool capturing = false;
    for(QKeySequenceEdit* edit : hotkeyEdits) {
        //  The editor's focus goes to its inner line edit, so check for children too.
        if(now && (now == edit || edit->isAncestorOf(now))) {
            capturing = true;
            break;
        }
    }
    if(capturing != hotkeyCapturing) {
        hotkeyCapturing = capturing;
        emit hotkeyCaptureChanged(capturing);
    }
}

//  Push to talk is the microphone hotkey, held down. The slider only has a threshold knob for voice activation,
//  and the microphone hotkey changes its name when it is push to talk.
void lmFormSettings::updateActivationUi(void) {
    const int mode = ui.cboMicActivation->currentIndex();
    ui.sldMicThreshold->setThresholdVisible(mode == MA_VoiceActivity);
    ui.sldMicThreshold->setExternalTrigger(mode == MA_PushToTalk);
    pPttTestTimer->stop();
    ui.lblMicHotkey->setText(mode == MA_PushToTalk ? tr("Push to talk:") : micHotkeyIdleText);
    validateHotkeys();
}

//  Shows what is wrong with the hotkeys, if anything, and blocks OK until it is fixed.
void lmFormSettings::validateHotkeys(void) {
    QString problem;
    QString pttProblem;
    if(lmHotkeyHandler::isSupported()) {
        if(ui.chkGlobalHotkeys->isChecked()) {
            for(int index = 0; index < lmHotkeyHandler::HK_Max && problem.isEmpty(); index++) {
                const QKeySequence sequence = hotkeyEdits[index]->keySequence();
                if(sequence.isEmpty())
                    continue;

                const QString name = lmHotkeyHandler::actionName(lmHotkeyHandler::Action(index));
                const QString reason = lmHotkeyHandler::validate(sequence);
                if(!reason.isEmpty()) {
                    problem = tr("%1: %2").arg(name, reason);
                    break;
                }
                for(int other = 0; other < index; other++) {
                    if(hotkeyEdits[other]->keySequence() == sequence) {
                        problem = tr("%1 and %2 cannot share the same hotkey.")
                                      .arg(lmHotkeyHandler::actionName(lmHotkeyHandler::Action(other)), name);
                        break;
                    }
                }
            }
        }
        //  Without a key there would be no way to talk.
        if(ui.cboMicActivation->currentIndex() == MA_PushToTalk) {
            if(!ui.chkGlobalHotkeys->isChecked())
                pttProblem = tr("Push to talk needs global hotkeys. Enable them on the Hotkeys page.");
            else if(hotkeyEdits[lmHotkeyHandler::HK_Mic]->keySequence().isEmpty())
                pttProblem = tr("Push to talk needs a key. Set one on the Hotkeys page.");
        }
    }

    const QString shown = problem.isEmpty() ? pttProblem : problem;
    ui.lblHotkeyWarning->setText(shown);
    ui.lblHotkeyWarning->setVisible(!shown.isEmpty());
    ui.lblMicWarning->setText(pttProblem);
    ui.lblMicWarning->setVisible(!pttProblem.isEmpty());
    ui.btnOK->setEnabled(shown.isEmpty());
    ui.btnOK->setToolTip(shown);
}

void lmFormSettings::setPageHeaderStyle(QLabel* pLabel) {
    QFont font = pLabel->font();
    int fontSize = pLabel->fontInfo().pixelSize();
    fontSize += (fontSize * 0.2);
    font.setPixelSize(fontSize);
    font.setBold(true);
    pLabel->setFont(font);
}

void lmFormSettings::setUIText(void) {
    ui.retranslateUi(this);

    setWindowTitle(tr("Preferences"));

    ui.chkAutoStart->setText(ui.chkAutoStart->text().arg(lmStrings::appName()));
    ui.chkAutoShow->setText(ui.chkAutoShow->text().arg(lmStrings::appName()));
    ui.lblFinePrint->setText(ui.lblFinePrint->text().arg(lmStrings::appName()));
    ui.lblHotkeyDesc->setText(ui.lblHotkeyDesc->text().arg(lmStrings::appName()));
    micTestIdleText = ui.btnTestMic->text();
    micHotkeyIdleText = ui.lblMicHotkey->text();
    if(micTesting)
        ui.btnTestMic->setText(tr("Stop"));

    if(!QSystemTrayIcon::isSystemTrayAvailable()) {
        ui.grpSysTray->setEnabled(false);
        ui.grpSysTray->setTitle(tr("System Tray (Not Available)"));
    }
    if(!QSystemTrayIcon::supportsMessages()) {
        ui.grpAlerts->setEnabled(false);
        ui.grpAlerts->setTitle(tr("Status Alerts (Not Available)"));
    }
    if(!lmSoundPlayer::isAvailable()) {
        ui.grpSounds->setEnabled(false);
        ui.grpSounds->setTitle(tr("Sounds (Not Available)"));
    }
    if(!lmHotkeyHandler::isSupported()) {
        ui.grpCallHotkeys->setEnabled(false);
        ui.grpCallHotkeys->setTitle(tr("Calls (Not Available)"));
    }

    for(int index = 0; index < ui.cboFontSize->count(); index++)
        ui.cboFontSize->setItemText(index, lmStrings::fontSize()[index]);

    for(int index = 0; index < ui.lvSounds->count(); index++)
        ui.lvSounds->item(index)->setText(lmStrings::soundDesc()[index]);

    for(int index = 0; index < ULV_Max; index++)
        ui.cboUserListView->setItemText(index, lmStrings::userListView()[index]);

    cboTheme_currentIndexChanged(ui.cboTheme->currentIndex());

    //	device lists contain translated entries, rebuild them (on first run loadSettings fills them)
    if(ui.cboMicDevice->count() > 0)
        populateMicDevices();
    if(ui.cboCamDevice->count() > 0)
        populateCamDevices();

#ifdef Q_OS_MAC
    ui.rdbEnter->setText("Return");
    ui.rdbCmdEnter->setText(QString(QChar(0x2318)) + " + Return"); // U+2318 is the hex code for Bowen Knot symbol
#else
    ui.rdbEnter->setText("Enter");
    ui.rdbCmdEnter->setText("Ctrl + Enter");
#endif

    updateActivationUi();

    //	set minimum possible size
    layout()->setSizeConstraint(QLayout::SetMinimumSize);
}

void lmFormSettings::loadSettings(void) {
    //	Auto start function not implemented on Mac since Mac itself provides an easy UI for it
#ifdef Q_OS_MAC
    ui.chkAutoStart->setChecked(false);
    ui.chkAutoStart->hide();
#else
    ui.chkAutoStart->setChecked(pSettings->value(IDS_AUTOSTART, IDS_AUTOSTART_VAL).toBool());
#endif
    ui.chkAutoShow->setChecked(pSettings->value(IDS_AUTOSHOW, IDS_AUTOSHOW_VAL).toBool());
    ui.chklmDebugLog->setChecked(lmDebugLog::isEnabled());    //Live state, so it also reflects --trace
    ui.chkSysTray->setChecked(pSettings->value(IDS_SYSTRAY, IDS_SYSTRAY_VAL).toBool());
    ui.chkMinimizeTray->setChecked(pSettings->value(IDS_MINIMIZETRAY, IDS_MINIMIZETRAY_VAL).toBool());
    ui.chkSingleClickTray->setChecked(pSettings->value(IDS_SINGLECLICKTRAY, IDS_SINGLECLICKTRAY_VAL).toBool());
    ui.chkSysTrayMsg->setChecked(pSettings->value(IDS_SYSTRAYMSG, IDS_SYSTRAYMSG_VAL).toBool());
    ui.chkAllowSysTrayMin->setChecked(pSettings->value(IDS_ALLOWSYSTRAYMIN, IDS_ALLOWSYSTRAYMIN_VAL).toBool());
    QString langCode = pSettings->value(IDS_LANGUAGE, IDS_LANGUAGE_VAL).toString();
    for(int index = 0; index < ui.cboLanguage->count(); index ++) {
        QString code = ui.cboLanguage->itemData(index, Qt::UserRole).toString();
        if(langCode.compare(code) == 0) {
            ui.cboLanguage->setCurrentIndex(index);
            break;
        }
    }

    ui.txtUserName->setText(pSettings->value(IDS_USERNAME, IDS_USERNAME_VAL).toString());
    ui.txtUserGroup->setText(Helper::normalizeGroupName(pSettings->value(IDS_USERGROUP, IDS_USERGROUP_VAL).toString()));
    ui.txtFirstName->setText(pSettings->value(IDS_USERFIRSTNAME, IDS_USERFIRSTNAME_VAL).toString());
    ui.txtLastName->setText(pSettings->value(IDS_USERLASTNAME, IDS_USERLASTNAME_VAL).toString());
    ui.txtAbout->setPlainText(pSettings->value(IDS_USERABOUT, IDS_USERABOUT_VAL).toString());
    ui.spnRefreshTime->setValue(pSettings->value(IDS_REFRESHTIME, IDS_REFRESHTIME_VAL).toInt());

    ui.rdbMessageTop->setChecked(pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
    ui.rdbMessageBottom->setChecked(!pSettings->value(IDS_MESSAGETOP, IDS_MESSAGETOP_VAL).toBool());
    ui.chkPublicMessagePop->setChecked(pSettings->value(IDS_PUBMESSAGEPOP, IDS_PUBMESSAGEPOP_VAL).toBool());
    ui.chkEmoticon->setChecked(pSettings->value(IDS_EMOTICON, IDS_EMOTICON_VAL).toBool());
    ui.chkMessageTime->setChecked(pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool());
    chkMessageTime_toggled(pSettings->value(IDS_MESSAGETIME, IDS_MESSAGETIME_VAL).toBool());
    ui.chkMessageDate->setChecked(pSettings->value(IDS_MESSAGEDATE, IDS_MESSAGEDATE_VAL).toBool());
    ui.chkAllowLinks->setChecked(pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool());
    chkAllowLinks_toggled(pSettings->value(IDS_ALLOWLINKS, IDS_ALLOWLINKS_VAL).toBool());
    ui.chkPathToLink->setChecked(pSettings->value(IDS_PATHTOLINK, IDS_PATHTOLINK_VAL).toBool());
    ui.chkTrimMessage->setChecked(pSettings->value(IDS_TRIMMESSAGE, IDS_TRIMMESSAGE_VAL).toBool());
    ui.chkClearOnClose->setChecked(pSettings->value(IDS_CLEARONCLOSE, IDS_CLEARONCLOSE_VAL).toBool());
    fontSize = pSettings->value(IDS_FONTSIZE, IDS_FONTSIZE_VAL).toInt();
    fontSize = qMin(FS_LARGE, qMax(FS_SMALL, fontSize));
    ui.cboFontSize->setCurrentIndex(fontSize);

    ui.chkHistory->setChecked(pSettings->value(IDS_HISTORY, IDS_HISTORY_VAL).toBool());
    ui.rdbSysHistoryPath->setChecked(pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
    ui.rdbCustomHistoryPath->setChecked(!pSettings->value(IDS_SYSHISTORYPATH, IDS_SYSHISTORYPATH_VAL).toBool());
    ui.txtHistoryPath->setText(History::historyFile());
    ui.chkFileHistory->setChecked(pSettings->value(IDS_FILEHISTORY, IDS_FILEHISTORY_VAL).toBool());

    ui.chkAlert->setChecked(pSettings->value(IDS_ALERT, IDS_ALERT_VAL).toBool());
    ui.chkNoBusyAlert->setChecked(pSettings->value(IDS_NOBUSYALERT, IDS_NOBUSYALERT_VAL).toBool());
    ui.chkNoDNDAlert->setChecked(pSettings->value(IDS_NODNDALERT, IDS_NODNDALERT_VAL).toBool());
    ui.chkSound->setChecked(pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool());
    emit ui.chkSound->toggled(pSettings->value(IDS_SOUND, IDS_SOUND_VAL).toBool());
    // Check so that number of elements read from settings file does not exceed the number of elements
    // in the list view control. This prevents array out of bounds error.
    int size = qMin(pSettings->beginReadArray(IDS_SOUNDEVENTHDR), ui.lvSounds->count());
    for(int index = 0; index < size; index++) {
        pSettings->setArrayIndex(index);
        ui.lvSounds->item(index)->setCheckState((Qt::CheckState)pSettings->value(IDS_SOUNDEVENT).toInt());
    }
    pSettings->endArray();
    size = qMin(pSettings->beginReadArray(IDS_SOUNDFILEHDR), ui.lvSounds->count());
    for(int index = 0; index < size; index++) {
        pSettings->setArrayIndex(index);
        ui.lvSounds->item(index)->setData(Qt::UserRole, pSettings->value(IDS_SOUNDFILE).toString());
    }
    pSettings->endArray();
    ui.chkNoBusySound->setChecked(pSettings->value(IDS_NOBUSYSOUND, IDS_NOBUSYSOUND_VAL).toBool());
    ui.chkNoDNDSound->setChecked(pSettings->value(IDS_NODNDSOUND, IDS_NODNDSOUND_VAL).toBool());

    ui.spnTimeout->setValue(pSettings->value(IDS_TIMEOUT, IDS_TIMEOUT_VAL).toInt());
    ui.spnMaxRetries->setValue(pSettings->value(IDS_MAXRETRIES, IDS_MAXRETRIES_VAL).toInt());
    size = pSettings->beginReadArray(IDS_BROADCASTHDR);
    for(int index = 0; index < size; index++) {
        pSettings->setArrayIndex(index);
        QListWidgetItem* item = new QListWidgetItem(ui.lvBroadcasts);
        item->setText(pSettings->value(IDS_BROADCAST).toString());
    }
    pSettings->endArray();
    ui.txtMulticast->setText(pSettings->value(IDS_MULTICAST, IDS_MULTICAST_VAL).toString());
    ui.txtUDPPort->setText(pSettings->value(IDS_UDPPORT, IDS_UDPPORT_VAL).toString());
    ui.txtTCPPort->setText(pSettings->value(IDS_TCPPORT, IDS_TCPPORT_VAL).toString());

    ui.chkAutoFile->setChecked(pSettings->value(IDS_AUTOFILE, IDS_AUTOFILE_VAL).toBool());
    ui.chkAutoShowFile->setChecked(pSettings->value(IDS_AUTOSHOWFILE, IDS_AUTOSHOWFILE_VAL).toBool());
    chkAutoShowFile_toggled(pSettings->value(IDS_AUTOSHOWFILE, IDS_AUTOSHOWFILE_VAL).toBool());
    ui.rdbFileTop->setChecked(pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool());
    ui.rdbFileBottom->setChecked(!pSettings->value(IDS_FILETOP, IDS_FILETOP_VAL).toBool());
    ui.txtFilePath->setText(DefinitionsDir::fileStorageDir());

    int colorSchemeIndex = pSettings->value(IDS_COLORSCHEME, IDS_COLORSCHEME_VAL).toInt();
    ui.cboTheme->setCurrentIndex(colorSchemeIndex);

    int userListView = pSettings->value(IDS_USERLISTVIEW, IDS_USERLISTVIEW_VAL).toInt();
    ui.cboUserListView->setCurrentIndex(userListView);
    ui.chkUserListToolTip->setChecked(pSettings->value(IDS_STATUSTOOLTIP, IDS_STATUSTOOLTIP_VAL).toBool());

    ui.rdbEnter->setChecked(!pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());
    ui.rdbCmdEnter->setChecked(pSettings->value(IDS_SENDKEYMOD, IDS_SENDKEYMOD_VAL).toBool());

    micDeviceId = pSettings->value(IDS_MICDEVICE, IDS_MICDEVICE_VAL).toString().toUtf8();
    micDeviceName = pSettings->value(IDS_MICDEVICENAME, IDS_MICDEVICENAME_VAL).toString();
    populateMicDevices();
    camDeviceKey = pSettings->value(IDS_CAMDEVICE, IDS_CAMDEVICE_VAL).toString();
    camDeviceName = pSettings->value(IDS_CAMDEVICENAME, IDS_CAMDEVICENAME_VAL).toString();
    populateCamDevices();
    ui.sldMicThreshold->setValue(pSettings->value(IDS_MICTHRESHOLD, IDS_MICTHRESHOLD_VAL).toInt());
    ui.cboMicActivation->setCurrentIndex(qBound(0, pSettings->value(IDS_MICACTIVATION, IDS_MICACTIVATION_VAL).toInt(), int(MA_Max) - 1));

    ui.chkGlobalHotkeys->setChecked(pSettings->value(IDS_GLOBALHOTKEYS, IDS_GLOBALHOTKEYS_VAL).toBool());
    for(int index = 0; index < lmHotkeyHandler::HK_Max; index++)
        hotkeyEdits[index]->setKeySequence(lmHotkeyHandler::loadSequence(pSettings, lmHotkeyHandler::Action(index)));
    chkGlobalHotkeys_toggled(ui.chkGlobalHotkeys->isChecked());
}

void lmFormSettings::saveSettings(void) {
    pSettings->setValue(IDS_VERSION, IDA_VERSION);

    pSettings->setValue(IDS_AUTOSTART, ui.chkAutoStart->isChecked(), IDS_AUTOSTART_VAL);
    pSettings->setValue(IDS_AUTOSHOW, ui.chkAutoShow->isChecked(), IDS_AUTOSHOW_VAL);
    if(ui.chklmDebugLog->isChecked() != lmDebugLog::isEnabled()) {    //Untouched box must not persist a --trace session
        pSettings->setValue(IDS_DEBUGLOG, ui.chklmDebugLog->isChecked(), IDS_DEBUGLOG_VAL);
        lmDebugLog::setEnabled(ui.chklmDebugLog->isChecked());
    }
    pSettings->setValue(IDS_SYSTRAY, ui.chkSysTray->isChecked(), IDS_SYSTRAY_VAL);
    pSettings->setValue(IDS_MINIMIZETRAY, ui.chkMinimizeTray->isChecked(), IDS_MINIMIZETRAY_VAL);
    pSettings->setValue(IDS_SINGLECLICKTRAY, ui.chkSingleClickTray->isChecked(), IDS_SINGLECLICKTRAY_VAL);
    pSettings->setValue(IDS_SYSTRAYMSG, ui.chkSysTrayMsg->isChecked(), IDS_SYSTRAYMSG_VAL);
    pSettings->setValue(IDS_ALLOWSYSTRAYMIN, ui.chkAllowSysTrayMin->isChecked(), IDS_ALLOWSYSTRAYMIN_VAL);
    QString langCode = ui.cboLanguage->itemData(ui.cboLanguage->currentIndex(), Qt::UserRole).toString();
    pSettings->setValue(IDS_LANGUAGE, langCode, IDS_LANGUAGE_VAL);

    pSettings->setValue(IDS_USERNAME, ui.txtUserName->text(), IDS_USERNAME_VAL);
    //  groups are case insensitive and capitalized per word, show the user what peers will see
    QString userGroup = Helper::normalizeGroupName(ui.txtUserGroup->text());
    ui.txtUserGroup->setText(userGroup);
    pSettings->setValue(IDS_USERGROUP, userGroup, IDS_USERGROUP_VAL);
    pSettings->setValue(IDS_USERFIRSTNAME, ui.txtFirstName->text(), IDS_USERFIRSTNAME_VAL);
    pSettings->setValue(IDS_USERLASTNAME, ui.txtLastName->text(), IDS_USERLASTNAME_VAL);
    pSettings->setValue(IDS_USERABOUT, ui.txtAbout->toPlainText(), IDS_USERABOUT_VAL);
    pSettings->setValue(IDS_REFRESHTIME, ui.spnRefreshTime->value(), IDS_REFRESHTIME_VAL);

    pSettings->setValue(IDS_MESSAGETOP, ui.rdbMessageTop->isChecked(), IDS_MESSAGETOP_VAL);
    pSettings->setValue(IDS_PUBMESSAGEPOP, ui.chkPublicMessagePop->isChecked(), IDS_PUBMESSAGEPOP_VAL);
    pSettings->setValue(IDS_EMOTICON, ui.chkEmoticon->isChecked(), IDS_EMOTICON_VAL);
    pSettings->setValue(IDS_MESSAGETIME, ui.chkMessageTime->isChecked(), IDS_MESSAGETIME_VAL);
    pSettings->setValue(IDS_MESSAGEDATE, ui.chkMessageDate->isChecked(), IDS_MESSAGEDATE_VAL);
    pSettings->setValue(IDS_ALLOWLINKS, ui.chkAllowLinks->isChecked(), IDS_ALLOWLINKS_VAL);
    pSettings->setValue(IDS_PATHTOLINK, ui.chkPathToLink->isChecked(), IDS_PATHTOLINK_VAL);
    pSettings->setValue(IDS_TRIMMESSAGE, ui.chkTrimMessage->isChecked(), IDS_TRIMMESSAGE_VAL);
    pSettings->setValue(IDS_CLEARONCLOSE, ui.chkClearOnClose->isChecked(), IDS_CLEARONCLOSE_VAL);
    pSettings->setValue(IDS_FONTSIZE, ui.cboFontSize->currentIndex(), IDS_FONTSIZE_VAL);

    pSettings->setValue(IDS_HISTORY, ui.chkHistory->isChecked(), IDS_HISTORY_VAL);
    pSettings->setValue(IDS_SYSHISTORYPATH, ui.rdbSysHistoryPath->isChecked(), IDS_SYSHISTORYPATH_VAL);
    pSettings->setValue(IDS_HISTORYPATH, ui.txtHistoryPath->text(), IDS_HISTORYPATH_VAL);
    pSettings->setValue(IDS_FILEHISTORY, ui.chkFileHistory->isChecked(), IDS_FILEHISTORY_VAL);

    pSettings->setValue(IDS_ALERT, ui.chkAlert->isChecked(), IDS_ALERT_VAL);
    pSettings->setValue(IDS_NOBUSYALERT, ui.chkNoBusyAlert->isChecked(), IDS_NOBUSYALERT_VAL);
    pSettings->setValue(IDS_NODNDALERT, ui.chkNoDNDAlert->isChecked(), IDS_NODNDALERT_VAL);
    pSettings->setValue(IDS_SOUND, ui.chkSound->isChecked(), IDS_SOUND_VAL);
    int checkCount = 0;
    int soundFileCount = 0;
    if(ui.lvSounds->count() > 0) {
        pSettings->beginWriteArray(IDS_SOUNDEVENTHDR);
        for(int index = 0; index < ui.lvSounds->count(); index++) {
            pSettings->setArrayIndex(index);
            pSettings->setValue(IDS_SOUNDEVENT, ui.lvSounds->item(index)->checkState());
            if(ui.lvSounds->item(index)->checkState() == IDS_SOUNDEVENT_VAL)
                checkCount++;
        }
        pSettings->endArray();
        pSettings->beginWriteArray(IDS_SOUNDFILEHDR);
        for(int index = 0; index < ui.lvSounds->count(); index++) {
            pSettings->setArrayIndex(index);
            pSettings->setValue(IDS_SOUNDFILE, ui.lvSounds->item(index)->data(Qt::UserRole).toString());
            if(ui.lvSounds->item(index)->data(Qt::UserRole).toString().compare(soundFile[index]) == 0)
                soundFileCount++;
        }
        pSettings->endArray();
    }
    if(ui.lvSounds->count() == 0 || checkCount == ui.lvSounds->count()) {
        pSettings->beginGroup(IDS_SOUNDEVENTHDR);
        pSettings->remove("");
        pSettings->endGroup();
    }
    if(ui.lvSounds->count() == 0 || soundFileCount == ui.lvSounds->count()) {
        pSettings->beginGroup(IDS_SOUNDFILEHDR);
        pSettings->remove("");
        pSettings->endGroup();
    }
    pSettings->setValue(IDS_NOBUSYSOUND, ui.chkNoBusySound->isChecked(), IDS_NOBUSYSOUND_VAL);
    pSettings->setValue(IDS_NODNDSOUND, ui.chkNoDNDSound->isChecked(), IDS_NODNDSOUND_VAL);

    pSettings->setValue(IDS_TIMEOUT, ui.spnTimeout->value(), IDS_TIMEOUT_VAL);
    pSettings->setValue(IDS_MAXRETRIES, ui.spnMaxRetries->value(), IDS_MAXRETRIES_VAL);
    //	If any broadcast address is specified, settings written to settings file
    //	Otherwise, the entire group is removed from the settings file
    if(ui.lvBroadcasts->count() > 0) {
        pSettings->beginWriteArray(IDS_BROADCASTHDR);
        for(int index = 0; index < ui.lvBroadcasts->count(); index++) {
            pSettings->setArrayIndex(index);
            pSettings->setValue(IDS_BROADCAST, ui.lvBroadcasts->item(index)->text());
        }
        pSettings->endArray();
    }
    if(ui.lvBroadcasts->count() == 0){
        pSettings->beginGroup(IDS_BROADCASTHDR);
        pSettings->remove("");
        pSettings->endGroup();
    }
    pSettings->setValue(IDS_MULTICAST, ui.txtMulticast->text(), IDS_MULTICAST_VAL);
    pSettings->setValue(IDS_UDPPORT, ui.txtUDPPort->text(), IDS_UDPPORT_VAL);
    pSettings->setValue(IDS_TCPPORT, ui.txtTCPPort->text(), IDS_TCPPORT_VAL);

    pSettings->setValue(IDS_AUTOFILE, ui.chkAutoFile->isChecked(), IDS_AUTOFILE_VAL);
    pSettings->setValue(IDS_AUTOSHOWFILE, ui.chkAutoShowFile->isChecked(), IDS_AUTOSHOWFILE_VAL);
    pSettings->setValue(IDS_FILETOP, ui.rdbFileTop->isChecked(), IDS_FILETOP_VAL);
    pSettings->setValue(IDS_FILESTORAGEPATH, ui.txtFilePath->text(), IDS_FILESTORAGEPATH_VAL);

    int colorSchemeIndex = ui.cboTheme->currentIndex();
    Helper::changeColorScheme(colorSchemeIndex);
    pSettings->setValue(IDS_COLORSCHEME, colorSchemeIndex, IDS_COLORSCHEME_VAL);
    pSettings->setValue(IDS_USERLISTVIEW, ui.cboUserListView->currentIndex(), IDS_USERLISTVIEW_VAL);
    pSettings->setValue(IDS_STATUSTOOLTIP, ui.chkUserListToolTip->isChecked(), IDS_STATUSTOOLTIP_VAL);

    pSettings->setValue(IDS_SENDKEYMOD, ui.rdbCmdEnter->isChecked(), IDS_SENDKEYMOD_VAL);
    pSettings->setValue(IDS_MICDEVICE, QString::fromUtf8(micDeviceId), IDS_MICDEVICE_VAL);
    pSettings->setValue(IDS_MICDEVICENAME, micDeviceId.isEmpty() ? QString() : micDeviceName, IDS_MICDEVICENAME_VAL);
    pSettings->setValue(IDS_CAMDEVICE, camDeviceKey, IDS_CAMDEVICE_VAL);
    pSettings->setValue(IDS_CAMDEVICENAME, camDeviceKey.isEmpty() ? QString() : camDeviceName, IDS_CAMDEVICENAME_VAL);
    pSettings->setValue(IDS_MICTHRESHOLD, ui.sldMicThreshold->value(), IDS_MICTHRESHOLD_VAL);
    pSettings->setValue(IDS_MICACTIVATION, ui.cboMicActivation->currentIndex(), IDS_MICACTIVATION_VAL);
    pSettings->setValue(IDS_GLOBALHOTKEYS, ui.chkGlobalHotkeys->isChecked(), IDS_GLOBALHOTKEYS_VAL);
    for(int index = 0; index < lmHotkeyHandler::HK_Max; index++)
        lmHotkeyHandler::saveSequence(pSettings, lmHotkeyHandler::Action(index), hotkeyEdits[index]->keySequence());

    pSettings->sync();
}

