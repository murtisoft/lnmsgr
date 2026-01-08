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


#ifndef AFORMUSERSELECT_H
#define AFORMUSERSELECT_H

#include <QDialog>
#include "ui_aformuserselect.h"
#include "shared.h"
#include "settings.h"

class lmFormUserSelect : public QDialog
{
    Q_OBJECT

public:
	lmFormUserSelect(QWidget *parent = 0);
	~lmFormUserSelect();

	void init(QList<QTreeWidgetItem*>* pContactsList);

	QStringList selectedContacts;

protected:
	void changeEvent(QEvent* pEvent);

private slots:
	void btnOK_clicked(void);
	void tvUserList_itemChanged(QTreeWidgetItem* item, int column);

private:
	void setUIText(void);

	Ui::UserSelectDialog ui;
	lmSettings* pSettings;
	bool parentToggling;
	bool childToggling;
	int selectedCount;
};

#endif // AFORMUSERSELECT_H
