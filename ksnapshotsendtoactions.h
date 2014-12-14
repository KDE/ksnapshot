/*
 *  Copyright (C) 2014 Gregor Mi <codestruct@posteo.org>
 *  moved from ksnapshot.h, see authors there
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

#ifndef KSNAPSHOTSENDTOACTIONS_H
#define KSNAPSHOTSENDTOACTIONS_H

#include "config-ksnapshot.h"
#include <QWidget>
#include <QAction>
#include <KService>

#ifdef KIPI_FOUND
class KSnapshot;
class KIPIInterface;
#include "ksnapshotimagecollectionshared.h"
#include <kipi/pluginloader.h>
#endif

class KSnapshotSendToActions : public QObject
{
    Q_OBJECT

public:
    explicit KSnapshotSendToActions();

    virtual ~KSnapshotSendToActions();

#ifdef KIPI_FOUND
    /**
     * needed for the current usage of the kipi interface
     * 2014-12-06 todo: this dependency can probably be removed
     */
    void setKSnapshotForKipi(KSnapshot *ksnapshot);
#endif

    /**
     * Fills the ActionList cache.
     */
    void retrieveActions();

    /**
     * Gets the action list retrieved by retrieveActions
     */
    QList<QAction*> actions();

private:
    bool retrieveActionsFunc();

    QList<QAction*> createSendToActions();

private:
    QList<QAction*> m_sendToActions;

#ifdef KIPI_FOUND
    KSnapshot *m_ksnapshot;
    KIPIInterface *m_kipiInterface;
    KIPI::PluginLoader *m_pluginLoader;
    friend QList<QUrl> KSnapshotImageCollectionShared::images();
#endif
};

class KSnapshotServiceAction : public QAction
{
    Q_OBJECT
public:
    KSnapshotServiceAction(KService::Ptr s, QObject *parent)
        : QAction(parent), service(s) {}

    KSnapshotServiceAction(KService::Ptr s,
                           const QString &text,
                           QObject *parent)
        : QAction(text, parent), service(s) {}

    KSnapshotServiceAction(KService::Ptr s,
                           const QIcon &icon,
                           const QString &text,
                           QObject *parent)
        : QAction(icon, text, parent), service(s) {}

    KService::Ptr service;
};

#endif // KSNAPSHOTSENDTOACTIONS_H
