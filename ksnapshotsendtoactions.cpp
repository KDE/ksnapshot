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

#include "ksnapshotsendtoactions.h"

#include <KMimeTypeTrader>
#include <QApplication>
#include <QDebug>

#ifdef KIPI_FOUND
#include <kipi/plugin.h>
#include <libkipi_version.h>
#include "kipiinterface.h"
#include "ksnapshot.h" // todo: remove this dependency when possible
#endif

KSnapshotSendToActions::KSnapshotSendToActions()
{
}

KSnapshotSendToActions::~KSnapshotSendToActions()
{
    // deletion of m_sendToActions' items is done automatically because the parent was set to "this" on items' creation
}

#ifdef KIPI_FOUND
void KSnapshotSendToActions::setKSnapshotForKipi(KSnapshot *ksnapshot)
{
    m_ksnapshot = ksnapshot;

    m_kipiInterface = new KIPIInterface(m_ksnapshot);

#if(KIPI_VERSION >= 0x020000)
    // call this here to avoid "QObject: Cannot create children for a parent that is in a different thread." warning
    m_pluginLoader = new KIPI::PluginLoader();
#else
    // 2014-12-05:
    // this will probably take a while but we do it here to avoid
    // "QObject: Cannot create children for a parent that is in a different thread." warning
    // todo: When can this be removed because it seems only needed for an old version of KIPI?
    m_pluginLoader = new KIPI::PluginLoader(QStringList(), m_kipiInterface, "");
#endif
}
#endif

void KSnapshotSendToActions::retrieveActions()
{
    // m_runResult = QtConcurrent::run(KSnapshotSendToActions::retrieveActionsFunc, this); // 2014-12-14: kipi code not ready for async retrieval
    retrieveActionsFunc();
}

QList< QAction* > KSnapshotSendToActions::actions()
{
    return m_sendToActions;
}

bool KSnapshotSendToActions::retrieveActionsFunc()
{
#ifdef KIPI_FOUND
#if(KIPI_VERSION >= 0x020000)
    m_pluginLoader->setInterface(m_kipiInterface);
    m_pluginLoader->init();
#else
    // nothing
#endif
#endif

    m_sendToActions.clear();
    m_sendToActions = createSendToActions();

    return true;
}

/*
 * todo later: maybe add a separator between KService and kipi items
 */
QList<QAction*> KSnapshotSendToActions::createSendToActions()
{
    const KService::List services = KMimeTypeTrader::self()->query("image/png");
    QMap<QString, KService::Ptr> imageApps;

    for (auto service: services) {
        imageApps.insert(service->name(), service);
    }

    QList<QAction*> actionList;

    for (auto service: imageApps) {
        QString name = service->name().replace('&', "&&");
        auto action = new KSnapshotServiceAction(service, QIcon::fromTheme(service->icon()), name, nullptr);
        action->setParent(this);
        actionList.append(action);
    }

#ifdef KIPI_FOUND
    KIPI::PluginLoader::PluginList pluginList = m_pluginLoader->pluginList();

    for (auto pluginInfo: pluginList) {
        if (!pluginInfo->shouldLoad()) {
            continue;
        }
        KIPI::Plugin *plugin = pluginInfo->plugin();
        if (!plugin) {
            qWarning() << "Plugin from library" << pluginInfo->library() << "failed to load";
            continue;
        }

        plugin->setup(m_ksnapshot);

        QList<QAction *> actions = plugin->actions();
        QSet<QAction *> exportActions;
        for (auto action: actions) {
            KIPI::Category category = plugin->category(action);
            if (category == KIPI::ExportPlugin) {
                exportActions << action;
            } else if (category == KIPI::ImagesPlugin) {
                // Horrible hack. Why are the print images and the e-mail images plugins in the same category as rotate and edit metadata!?
                // 2014-10-30: please file kipi bug and reference it here
                if (pluginInfo->library().contains("kipiplugin_printimages") || pluginInfo->library().contains("kipiplugin_sendimages")) {
                    exportActions << action;
                }
            }
        }

        Q_FOREACH(QAction* action, exportActions) {
            actionList.append(action);
        }

#pragma message("PORT TO FRAMEWORKS (?)")
//            plugin->actionCollection()->readShortcutSettings();

    }
#endif

    return actionList;
}
