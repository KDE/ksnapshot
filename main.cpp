/*
 *  Copyright (C) 1997-2002 Richard J. Moore <rich@kde.org>
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

#include <QApplication>

#include <KAboutData>
#include <KLocalizedString>
#include <QCommandLineParser>
#include <KDBusService>
#include <Kdelibs4ConfigMigrator>

#include "ksnapshotadaptor.h"
#include "ksnapshot.h"
#include "ksnapshot_options.h"
#include "config-ksnapshot.h"

int main(int argc, char **argv)
{
    // migrate configuration from kdelibs4 to kf5

    Kdelibs4ConfigMigrator migrate(QStringLiteral("ksnapshot"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("ksnapshotrc"));
    migrate.migrate();

    // set up the application

    QApplication app(argc, argv);

    app.setOrganizationDomain("kde.org");
    app.setApplicationName("ksnapshot");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    // set up the about data

    KLocalizedString::setApplicationDomain("ksnapshot");
    KAboutData aboutData("ksnapshot", i18n("KSnapshot"), KSNAPVERSION, i18n("KDE Screenshot Utility"), KAboutLicense::GPL,
                         i18n("(c) 1997-2008, Richard J. Moore,\n(c) 2000, Matthias Ettrich,\n(c) 2002-2014 Aaron J. Seigo"));
    aboutData.addAuthor("Aaron J. Seigo", QString(), "aseigo@kde.org");
    aboutData.addAuthor("Richard J. Moore", QString(), QString("rich@kde.org"));
    aboutData.addAuthor("Matthias Ettrich", QString(), "ettrich@kde.org");
    aboutData.addCredit("Nadeem Hasan", i18n("Region Grabbing\nReworked GUI"), "nhasan@kde.org");
    aboutData.addCredit("Marcus Hufgard", i18n("\"Open With\" function"), "Marcus.Hufgard@hufgard.de");
    aboutData.addCredit("Pau Garcia i Quiles", i18n("Free region grabbing, KIPI plugins support, port to Windows"),
                        "pgquiles@elpauer.org");

    KAboutData::setApplicationData(aboutData);

    // set up the command line options parser

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    addCommandLineOptions(parser); // Add our own options.
    parser.process(app);
    aboutData.processCommandLine(&parser);

    // decide whether to create top level window

    bool showTopLevel = false;
    KSnapshotObject::CaptureMode startingMode = KSnapshotObject::FullScreen;

    if (parser.isSet("current")) {
        startingMode = KSnapshotObject::WindowUnderCursor;
    } else if (parser.isSet("fullscreen")) {
        // we grab directly desktop => show dialogbox
        showTopLevel = true;
        startingMode = KSnapshotObject::FullScreen;
    } else if (parser.isSet("region")) {
        startingMode = KSnapshotObject::Region;
    } else if (parser.isSet("freeregion")) {
        startingMode = KSnapshotObject::FreeRegion;
    } else if (parser.isSet("child")) {
        startingMode = KSnapshotObject::ChildWindow;
    } else {
        showTopLevel = true;
    }

    // create the KSnapshot instance

    KSnapshot window(startingMode);

    // take care of the DBus activation and stuff

    KDBusService service;
    new KsnapshotAdaptor(&window);
    QDBusConnection::sessionBus().registerObject("/KSnapshot", &window);

    // show window if we have to and fire the main loop

    if (showTopLevel) {
        window.show();
    }
    return app.exec();
}