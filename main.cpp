#include <kapp.h>
#include "ksnapshot.h"
#include <kimgio.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

static const char *description = 
	I18N_NOOP("KDE Screenshot utility");

int main(int argc, char **argv)
{
  KAboutData aboutData( "ksnapshot", I18N_NOOP("KSnapshot"), 
    KSNAPVERSION, description, KAboutData::GPL, 
    "(c) 1997-1999, Richard J. Moore");
  aboutData.addAuthor("Richard J. Moore",0, "rich@kde.org");
  KCmdLineArgs::init( argc, argv, &aboutData );
  
  KApplication app;

  kimgioRegister();

  // Create top level window
  KSnapShot *toplevel= new KSnapShot();
  app.setMainWidget(toplevel);
  toplevel->show();
  return app.exec();
}

