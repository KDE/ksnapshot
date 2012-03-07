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

#include "kipiinterface.h"
#include "ksnapshotinfoshared.h"
#include "ksnapshotimagecollectionshared.h"
#include "ksnapshot.h"
#include "kipiimagecollectionselector.h"
#include <libkipi/uploadwidget.h>
#include <libkipi/imagecollectionshared.h>
#include <libkipi/imageinfo.h>
#include <libkipi/imageinfoshared.h>
#include <libkipi/plugin.h>
#include <libkipi/interface.h>
#include <libkipi/pluginloader.h>
#include <libkipi/imagecollection.h>
#include <libkipi/imagecollectionselector.h>

struct KIPIInterfacePrivate {
    KSnapshot *ksnapshot;
    KIPI::PluginLoader* pluginLoader;
};

KIPIInterface::KIPIInterface(KSnapshot* ksnapshot)
:KIPI::Interface(ksnapshot)
, d(new KIPIInterfacePrivate) {
    d->ksnapshot = ksnapshot;
}

KIPIInterface::~KIPIInterface() {
    delete d;
}

KIPI::ImageCollection KIPIInterface::currentAlbum() {
    return KIPI::ImageCollection(new KSnapshotImageCollectionShared(d->ksnapshot));
}

KIPI::ImageCollection KIPIInterface::currentSelection() {
    return KIPI::ImageCollection(new KSnapshotImageCollectionShared(d->ksnapshot));
}

QList<KIPI::ImageCollection> KIPIInterface::allAlbums() {
    QList<KIPI::ImageCollection> list;
    list << currentSelection();
    return list;
}

KIPI::ImageInfo KIPIInterface::info(const KUrl& url) {
    return KIPI::ImageInfo(new KSnapshotInfoShared(this,url));
}

bool KIPIInterface::addImage(const KUrl&, QString&)
{
    return true;
}
void KIPIInterface::delImage( const KUrl& ) 
{

}
void KIPIInterface::refreshImages( const KUrl::List& )
{
// TODO Implement?
}

KIPI::ImageCollectionSelector* KIPIInterface::imageCollectionSelector(QWidget *parent) {
    return new KIPIImageCollectionSelector(this, parent);
}

KIPI::UploadWidget* KIPIInterface::uploadWidget(QWidget *parent) {
    return (new KIPI::UploadWidget(parent));
}

int KIPIInterface::features() const {
    return KIPI::HostAcceptNewImages;
}
