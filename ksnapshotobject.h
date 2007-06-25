/*
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
 *  Copyright (C) 1997-2002 Richard J. Moore
 *  Copyright (C) 2000 Matthias Ettrich
 *  Copyright (C) 2002 Aaron J. Seigo
 *  Copyright (C) 2003 Nadeem Hasan
 *  Copyright (C) 2004 Bernd Brandstetter
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ fwo.ch>
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

#ifndef KSNAPSHOTOBJECT_H
#define KSNAPSHOTOBJECT_H

#include <KUrl>
#include <QPixmap>
class QWidget;
class RegionGrabber;
class QString;

class KSnapshotObject
{
public:
     enum CaptureMode { FullScreen=0, WindowUnderCursor=1, Region=2, ChildWindow=3 };
     KSnapshotObject();
     virtual ~KSnapshotObject();

     bool save( const QString &filename, QWidget* widget );
     bool save( const KUrl& url, QWidget *widget );
     bool saveEqual( const KUrl& url,QWidget *widget );

protected:
     void autoincFilename();
     virtual void refreshCaption(){}

     void changeUrl(const QString &newUrl);

     KUrl filename;
     RegionGrabber *rgnGrab;
     QWidget* grabber;
     QPixmap snapshot;
};

#endif
