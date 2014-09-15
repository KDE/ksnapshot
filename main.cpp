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

#include "ksnapshotadaptor.h"
#include "ksnapshot.h"
#include "ksnapshot_options.h"

#define KSNAPVERSION "0.8.2"

static const char description[] = I18N_NOOP("KDE Screenshot Utility");

int main(int argc, char **argv)
{
    KLocalizedString::setApplicationDomain("ksnapshot");
    KAboutData aboutData("ksnapshot", i18n("KSnapshot"), KSNAPVERSION, i18n(description), KAboutLicense::GPL,
                         i18n("(c) 1997-2008, Richard J. Moore,\n(c) 2000, Matthias Ettrich,\n(c) 2002-2014 Aaron J. Seigo"));
    aboutData.addAuthor("Richard J. Moore", QString(), QString("rich@kde.org"));
    aboutData.addAuthor("Matthias Ettrich", QString(), "ettrich@kde.org");
    aboutData.addAuthor("Aaron J. Seigo", QString(), "aseigo@kde.org");
    aboutData.addCredit("Nadeem Hasan", i18n("Region Grabbing\nReworked GUI"), "nhasan@kde.org");
    aboutData.addCredit("Marcus Hufgard", i18n("\"Open With\" function"), "Marcus.Hufgard@hufgard.de");
    aboutData.addCredit("Pau Garcia i Quiles", i18n("Free region grabbing, KIPI plugins support, port to Windows"),
                        "pgquiles@elpauer.org");

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    addCommandLineOptions(parser);    // Add our own options.

    // This is one of the applications that requires the "native" / X11 graphics backend to work.
    QApplication::setGraphicsSystem("native");

    // Create top level window
    KSnapshot *toplevel;
    bool showTopLevel = false;

    if (parser.isSet("current")) {
        toplevel = new KSnapshot(0, KSnapshotObject::WindowUnderCursor);
    } else if (parser.isSet("fullscreen")) {
        //we grad directly desktop => show dialogbox
        showTopLevel = true;
        toplevel = new KSnapshot(0, KSnapshotObject::FullScreen);
    } else if (parser.isSet("region")) {
        toplevel = new KSnapshot(0, KSnapshotObject::Region);
    } else if (parser.isSet("freeregion")) {
        toplevel = new KSnapshot(0, KSnapshotObject::FreeRegion);
    } else if (parser.isSet("child")) {
        toplevel = new KSnapshot(0, KSnapshotObject::ChildWindow);
    } else {
        showTopLevel = true;
        toplevel = new KSnapshot();
    }


    new KsnapshotAdaptor(toplevel);
    QDBusConnection::sessionBus().registerObject("/KSnapshot", toplevel);

    if (showTopLevel) {
        toplevel->show();
    }
    return app.exec();
}

