#include <kapp.h>
#include "ksnapshot.h"
#include <kimgio.h>
#include <klocale.h>
#include <kcmdlineargs.h>


static const char *description = 
	I18N_NOOP("KDE Screenshot utility");

int main(int argc, char **argv)
{
  KCmdLineArgs::init(argc, argv, "ksnapshot", description, KSNAPVERSION);

  KApplication app;

  kimgioRegister();

  // Create top level window
  KSnapShot *toplevel= new KSnapShot();
  app.setMainWidget(toplevel);
  toplevel->show();
  return app.exec();
}

