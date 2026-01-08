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


#ifndef IMAGEPICKER_H
#define IMAGEPICKER_H

#include <QTableWidget>
#include <QWidget>
#include <QWidgetAction>

class lmImagePicker : public QTableWidget {
    Q_OBJECT

public:
    lmImagePicker(QWidget *parent, QList<QString>* source, int picSize, int columns, int* selected, int actionIndex);
    ~lmImagePicker();

protected:
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void paintEvent(QPaintEvent* e);
    void leaveEvent(QEvent* e);

private:
    int* selected;
    int actionIndex;
    int max_col;

    QTableWidgetItem* hoverItem;
};

class lmImagePickerAction : public QWidgetAction {
public:
    lmImagePickerAction(QObject* parent, const QString source[], int sourceCount, int picSize, int columns, int* selected);
    ~lmImagePickerAction(void);

    void releaseWidget(QWidget* widget);
    QWidget* createWidget(QWidget* parent);

private:
    QList<QString>* source;
    int picSize;
    int columns;
    int* selected;
};


#endif // IMAGEPICKER_H
