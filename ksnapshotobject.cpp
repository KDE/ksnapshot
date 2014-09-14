/*
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006-2008 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
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

#include "ksnapshotobject.h"

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

//kde include
#include <KMessageBox>

#include <klocale.h>
#include <QTemporaryFile>
#include <KJobWidgets>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <QDebug>

//Qt include
#include <QRegExp>
#include <QApplication>
#include <QImageWriter>
#include <QMimeDatabase>
#include <QMimeType>

KSnapshotObject::KSnapshotObject()
: rgnGrab( 0 ),
  freeRgnGrab( 0 ),
  grabber( 0 )
{
}

KSnapshotObject::~KSnapshotObject()
{
    delete grabber;
}

void KSnapshotObject::autoincFilename()
{
    // Extract the filename from the path
    QString name= filename.fileName();

    // If the name contains a number then increment it
    QRegExp numSearch( "(^|[^\\d])(\\d+)" ); // we want to match as far left as possible, and when the number is at the start of the name

    // Does it have a number?
    int start = numSearch.lastIndexIn( name );
    if (start != -1) {
        // It has a number, increment it
        start = numSearch.pos( 2 ); // we are only interested in the second group
        QString numAsStr = numSearch.capturedTexts()[ 2 ];
        QString number = QString::number( numAsStr.toInt() + 1 );
        number = number.rightJustified( numAsStr.length(), '0' );
        name.replace( start, numAsStr.length(), number );
    }
    else {
        // no number
        start = name.lastIndexOf('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            name.insert(start, '1');
        }
        else {
            // no extension, just tack it on to the end
            name += '1';
        }
    }

    //Rebuild the path
    QUrl newUrl = filename;
    newUrl = newUrl.adjusted(QUrl::RemoveFilename);
    newUrl.setPath(newUrl.path() +  name );
    changeUrl( newUrl.url() );
}


void KSnapshotObject::changeUrl( const QString &url )
{
    QUrl newURL = QUrl( url );
    if ( newURL == filename )
        return;

    filename = newURL;
    refreshCaption();
}


// NOTE: widget == NULL if called from dbus interface
bool KSnapshotObject::save( const QString &filename, QWidget* widget )
{
    return save( QUrl( filename ), widget);
}

bool KSnapshotObject::save( const QUrl &url, QWidget *widget )
{
    // NOTE: widget == NULL if called from dbus interface
    //TODO: non-blocking
    // we only need to test for existence; details about the file are uninteresting, so 0 for third param
    KIO::StatJob *job = KIO::stat(url, KIO::StatJob::DestinationSide, 0);
    KJobWidgets::setWindow(job, widget);
    job->exec();
    if (!job->error()) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" , url.url(QUrl::PreferLocalFile));
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel( widget, text, title, KGuiItem(i18n("Overwrite")) ) )
        {
            return false;
        }
    }
    return saveEqual( url,widget );
}

bool KSnapshotObject::saveEqual( const QUrl &url,QWidget *widget )
{
    QMimeDatabase db;
    QString type = db.mimeTypeForUrl(url.fileName()).name();
    if (type.isEmpty()) {
        type = "PNG";
    }

    bool ok = false;

    if ( url.isLocalFile() ) {
        QFile output( url.toLocalFile() );
        if ( output.open( QFile::WriteOnly ) )
            ok = saveImage(&output, type.toLatin1());
    }
    else {
        QTemporaryFile tmpFile;
        if ( tmpFile.open() ) {
            if (saveImage(&tmpFile, type.toLatin1())) {
                // TODO: non-blocking
                // we only need to test for existence; details about the file are uninteresting, so 0 for third param
                KIO::StatJob *job = KIO::fileCopy(tmpFile.fileName(), url);
                KJobWidgets::setWindow(job, widget);
                job->exec();
                ok = !job->error();
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if ( !ok ) {
        qWarning() << "KSnapshot was unable to save the snapshot" ;

        const QString caption = i18n("Unable to Save Image");
        const QString text = i18n("KSnapshot was unable to save the image to\n%1.", url.toDisplayString());
        KMessageBox::error(widget, text, caption);
    }

    return ok;
}

bool KSnapshotObject::saveImage( QIODevice *device, const QByteArray &format )
{
    QImageWriter imgWriter( device, format );

    if ( !imgWriter.canWrite() ) {
	//qDebug() << "Cannot write format " << format;
	return false;
    }

    // For jpeg use 85% quality not the default
    if ( 0 == qstricmp(format.constData(), "jpeg") || 0 == qstricmp(format.constData(), "jpg") ) {
	imgWriter.setQuality( 85 );
    }

    if ( !title.isEmpty() )
	imgWriter.setText( i18n("Title"), title );
    if ( !windowClass.isEmpty() )
	imgWriter.setText( i18n("Window Class"), windowClass );

    QImage snap = snapshot.toImage();
    return imgWriter.write( snap );
}

