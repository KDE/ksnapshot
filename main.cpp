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
  aboutData.addCredit( "Nadeem Hasan", "Region Grabbing\nReworked GUI",
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

  app.dcopClient()->setDefaultObject( toplevel->objId() );
  toplevel->setCaption( app.makeStdCaption("") );
  app.setMainWidget(toplevel);
  toplevel->show();
  return app.exec();
}

