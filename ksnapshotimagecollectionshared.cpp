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

#include "ksnapshotimagecollectionshared.h"
#include <KUrl>
#include <QtGlobal>
#include <QDebug>
#include "ksnapshot.h"

struct KSnapshotImageCollectionShared::Private
{
        KSnapshot* ksnapshot;
};

KSnapshotImageCollectionShared::KSnapshotImageCollectionShared(KSnapshot* ksnapshot): d(new Private), mImages(KUrl::List())
{
        d->ksnapshot = ksnapshot;
}

KSnapshotImageCollectionShared::~KSnapshotImageCollectionShared()
{
    delete d;
}

KUrl::List KSnapshotImageCollectionShared::images()
{
    // TODO (pgquiles) Clean up this mess. I'm not even sure all this is required!
    bool isTempfile = false;
    KUrl url = d->ksnapshot->urlToOpen(&isTempfile);
    if (!url.isValid())
    {
        return KUrl::List();
    }
    return KUrl::List(KUrl(url));
}
