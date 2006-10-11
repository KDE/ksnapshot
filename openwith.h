/*
 *  Copyright (C) 2006 Marcus Hufgard <marcus.Hufgard@hufgard.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef OPENWITH_H
#define OPENWITH_H

#include <QListView>

#include <kdialog.h>

class AppListView;

class OpenWith : public KDialog
{
    Q_OBJECT

public:
    OpenWith(const QString& mimeType, const QString& value, QWidget *parent = 0L);
    ~OpenWith();
    void runSelected();

protected slots:
    void otherApplication();
    void accept();

private:
    QString mime;
    QString value1;
    AppListView *applistview;
};

class AppListView : public QListView
{
    Q_OBJECT

public:
    AppListView(OpenWith* dlg, QWidget* parent=0L);
    ~AppListView();

protected slots:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void mouseDoubleClickEvent(QMouseEvent* event);
    OpenWith* openwdlg;
};

#endif // OPENWITH_H
