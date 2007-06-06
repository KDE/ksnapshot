/*
  (c) Richard J. Moore 1997-2002
  (c) Matthias Ettrich 2000
  (c) Aaron J. Seigo 2002-2004
  (c) Nadeem Hasan 2003
  (c) Waldo Bastian 1999-2002

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or ( at your option ) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include <kapplication.h>
#include <kimageio.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kiconloader.h>

#include "ksnapshot.h"

static const char description[] =
	I18N_NOOP("KDE Screenshot Utility");

static KCmdLineOptions options[] =
{
    { "c", 0, 0 },
    { "current", I18N_NOOP("Captures the window under the mouse on startup (instead of the desktop)"), 0 },
    { 0, 0, 0 }
};

int main(int argc, char **argv)
{
  KAboutData aboutData( "ksnapshot", I18N_NOOP("KSnapshot"),
    KSNAPVERSION, description, KAboutData::License_GPL,
    "(c) 1997-2004, Richard J. Moore,\n(c) 2000, Matthias Ettrich,\n(c) 2002-2003 Aaron J. Seigo");
  aboutData.addAuthor("Richard J. Moore",0, "rich@kde.org");
  aboutData.addAuthor("Matthias Ettrich",0, "ettrich@kde.org");
  aboutData.addAuthor("Aaron J. Seigo", 0, "aseigo@kde.org");
  aboutData.addCredit( "Nadeem Hasan", I18N_NOOP("Region Grabbing\nReworked GUI"),
      "nhasan@kde.org" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;

  KImageIO::registerFormats();

  // Create top level window
  KSnapshot *toplevel;

  if ( args->isSet( "current" ) )
     toplevel = new KSnapshot( 0, 0, true );
  else
     toplevel = new KSnapshot();

  args->clear();
  app.dcopClient()->setDefaultObject( toplevel->objId() );
  toplevel->setCaption( app.makeStdCaption("") );
  app.setMainWidget(toplevel);
  toplevel->show();
  return app.exec();
}

