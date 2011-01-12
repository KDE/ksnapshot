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


#include <kapplication.h>
#include <kimageio.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kiconloader.h>

#include "ksnapshotadaptor.h"
#include "ksnapshot.h"
#include "ksnapshot_options.h"

#define KSNAPVERSION "0.8.2"

static const char description[] = I18N_NOOP("KDE Screenshot Utility");

int main(int argc, char **argv)
{
  KAboutData aboutData( "ksnapshot", 0, ki18n("KSnapshot"),
    KSNAPVERSION, ki18n(description), KAboutData::License_GPL,
    ki18n("(c) 1997-2008, Richard J. Moore,\n(c) 2000, Matthias Ettrich,\n(c) 2002-2003 Aaron J. Seigo"));
  aboutData.addAuthor(ki18n("Richard J. Moore"),KLocalizedString(), "rich@kde.org");
  aboutData.addAuthor(ki18n("Matthias Ettrich"),KLocalizedString(), "ettrich@kde.org");
  aboutData.addAuthor(ki18n("Aaron J. Seigo"), KLocalizedString(), "aseigo@kde.org");
  aboutData.addCredit( ki18n("Nadeem Hasan"), ki18n("Region Grabbing\nReworked GUI"),
      "nhasan@kde.org" );
  aboutData.addCredit( ki18n("Marcus Hufgard"), ki18n("\"Open With\" function"),
      "Marcus.Hufgard@hufgard.de" );
  aboutData.addCredit( ki18n("Pau Garcia i Quiles"), ki18n("Free region grabbing, KIPI plugins support, port to Windows"),
      "pgquiles@elpauer.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( ksnapshot_options() ); // Add our own options.
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // This is one of the applications that requires the "native" / X11 graphics backend to work.
  QApplication::setGraphicsSystem("native");
  KApplication app;

  // Create top level window
  KSnapshot *toplevel;
  bool showTopLevel = false;

  if ( args->isSet( "current" ) )
     toplevel = new KSnapshot( 0, KSnapshotObject::WindowUnderCursor );
  else if(args->isSet( "fullscreen" ))
  {
     //we grad directly desktop => show dialogbox
     showTopLevel = true;
     toplevel = new KSnapshot( 0, KSnapshotObject::FullScreen );
  }
  else if(args->isSet( "region" ))
     toplevel = new KSnapshot( 0, KSnapshotObject::Region );
  else if(args->isSet( "freeregion" ))
     toplevel = new KSnapshot( 0, KSnapshotObject::FreeRegion );
  else if(args->isSet( "child" ))
     toplevel = new KSnapshot( 0, KSnapshotObject::ChildWindow );
  else
  {
     showTopLevel = true;
     toplevel = new KSnapshot();
  }

  args->clear();
  new KsnapshotAdaptor(toplevel);
  QDBusConnection::sessionBus().registerObject("/KSnapshot", toplevel);

  if(showTopLevel)
     toplevel->show();
  return app.exec();
}

