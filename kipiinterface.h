/*
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
 *  Essentially a rip-off of code for Kamoso by:
 *  Copyright (C) 2008-2009 by Aleix Pol <aleixpol@kde.org>
 *  Copyright (C) 2008-2009 by Alex Fiestas <alex@eyeos.org>

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

#ifndef KIPIINTERFACE_H
#define KIPIINTERFACE_H

#include <QList>
#include <kipi/interface.h>
#include <kipi/uploadwidget.h>
#include <kipi/imagecollectionshared.h>
#include <libkipi_version.h>

class KSnapshot;
struct KIPIInterfacePrivate;

#ifndef KIPI_VERSION_MAJOR
#error KIPI_VERSION_MAJOR should be provided.
#endif
#if KIPI_VERSION_MAJOR >= 5
#define KSNAPSHOT_KIPI_WITH_CREATE_METHODS
#endif

class KIPIInterface : public KIPI::Interface
{
    Q_OBJECT

public:
    KIPIInterface(KSnapshot *ksnapshot);
    virtual ~KIPIInterface();

    virtual bool addImage(const QUrl &, QString &err);
    virtual void delImage(const QUrl &);
    virtual void refreshImages(const QList<QUrl> &urls);

    virtual KIPI::ImageCollectionSelector *imageCollectionSelector(QWidget *parent);
    virtual KIPI::UploadWidget *uploadWidget(QWidget *parent);

    virtual QList<KIPI::ImageCollection> allAlbums();
    virtual KIPI::ImageCollection currentAlbum();
    virtual KIPI::ImageCollection currentSelection();
    virtual int features() const;
    virtual KIPI::ImageInfo info(const QUrl &);

#ifdef KSNAPSHOT_KIPI_WITH_CREATE_METHODS
    virtual KIPI::FileReadWriteLock* createReadWriteLock(const QUrl& url) const;
    virtual KIPI::MetadataProcessor* createMetadataProcessor() const;
#endif

private:
    KIPIInterfacePrivate *const d;
};

#endif // KIPIINTERFACE_H
