/*
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006-2008 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
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

#include <fixx11h.h>
//kde include
#include <KMessageBox>
#include <KMimeType>
#include <KImageIO>
#include <klocale.h>
#include <KTemporaryFile>
#include <kio/netaccess.h>
#include <kdebug.h>

//Qt include
#include <QRegExp>
#include <QApplication>
#include <QImageWriter>

KSnapshotObject::KSnapshotObject()
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
        name.replace( start, number.length(), number );
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
    KUrl newUrl = filename;
    newUrl.setFileName( name );
    changeUrl( newUrl.url() );
}


void KSnapshotObject::changeUrl( const QString &url )
{
    KUrl newURL = KUrl( url );
    if ( newURL == filename )
        return;

    filename = newURL;
    refreshCaption();
}


bool KSnapshotObject::save( const QString &filename, QWidget* widget )
{
    return save( KUrl( filename ), widget);
}

bool KSnapshotObject::save( const KUrl& url, QWidget *widget )
{
    if ( KIO::NetAccess::exists( url, KIO::NetAccess::DestinationSide, widget ) ) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" , url.prettyUrl());
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel( widget, text, title, KGuiItem(i18n("Overwrite")) ) )
        {
            return false;
        }
    }
    return saveEqual( url,widget );
}

bool KSnapshotObject::saveEqual( const KUrl& url,QWidget *widget )
{
    QByteArray type = "PNG";
    QString mime = KMimeType::findByUrl( url.fileName(), 0, url.isLocalFile(), true )->name();
    QStringList types = KImageIO::typeForMime(mime);
    if ( !types.isEmpty() )
        type = types.first().toLatin1();

    bool ok = false;

    if ( url.isLocalFile() ) {
	bool supported = false;
	foreach ( const QByteArray &format, QImageWriter::supportedImageFormats() ) {
	    if ( format.toLower() == type.toLower() )
		supported = true;
	}
        if ( supported && snapshot.save( url.path(), type ) )
            ok = true;
    }
    else {
        KTemporaryFile tmpFile;
        if ( tmpFile.open() ) {
            if ( snapshot.save( &tmpFile, type ) ) {
                ok = KIO::NetAccess::upload( tmpFile.fileName(), url, widget );
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if ( !ok ) {
        kWarning() << "KSnapshot was unable to save the snapshot" ;

        QString caption = i18n("Unable to Save Image");
        QString text = i18n("KSnapshot was unable to save the image to\n%1.", url.prettyUrl());
        KMessageBox::error(widget, text, caption);
    }

    return ok;
}
