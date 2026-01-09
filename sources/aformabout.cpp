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

#include <QFile>
#include "aformabout.h"

//	constructor
lmFormAbout::lmFormAbout(QWidget *parent, Qt::WindowFlags flags) : QDialog(parent, flags) {
    ui.setupUi(this);
    //	set minimum size
    layout()->setSizeConstraint(QLayout::SetMinimumSize);
    //	remove the help button from window button group
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //	Destroy the window when it closes
    setAttribute(Qt::WA_DeleteOnClose, true);
}

lmFormAbout::~lmFormAbout(void) {
}

void lmFormAbout::init(void) {
    setWindowIcon(QIcon(IDR_APPICON));

    pSettings = new lmSettings();
    setUIText();

    ui.tabWidget->setCurrentIndex(0);
}

void lmFormAbout::settingsChanged(void) {
}

void lmFormAbout::changeEvent(QEvent* pEvent) {
    switch(pEvent->type()) {
    case QEvent::LanguageChange:
        setUIText();
        break;
    default:
        break;
    }

    QDialog::changeEvent(pEvent);
}

void lmFormAbout::setUIText(void) {
    ui.retranslateUi(this);

    QString title = tr("About %1");
    setWindowTitle(title.arg(lmStrings::appName()));

    ui.lblTitle->setText(lmStrings::appName() + "\n" IDA_VERSION);
    ui.lblLogoSmall->setPixmap(QPixmap(IDR_LOGOSMALL));
#if defined(QT_NO_DEBUG)
#define DEBUGINFO " "
#else
#define DEBUGINFO " debug"
#endif
    ui.lblQtVersion->setText(QString("Qt %1  %2" DEBUGINFO "\n%3")
                             .arg(QT_VERSION_STR,
                                  QSysInfo::buildCpuArchitecture(),
                                  QSysInfo::prettyProductName()));

    QString description(lmStrings::appDesc() + "\n\n");
    ui.lblDescription->setText(description);


    QFile thanks(":/credits/contributors");
    if(thanks.open(QIODevice::ReadOnly)) {
        ui.txtThanks->setHtml(QString::fromUtf8(thanks.readAll()));
        thanks.close();
    }

    QFile license(":/credits/index");
    if(license.open(QIODevice::ReadOnly)) {
        ui.txtLicense->setHtml(QString::fromUtf8(license.readAll()));
        license.close();
    }
}
