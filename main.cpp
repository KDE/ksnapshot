#include <kapp.h>
#include "formats.h"
#include "ksnapshot.h"

FormatManager *formatMngr;

int main(int argc, char **argv)
{
  KApplication *app;
  KSnapShot *toplevel;

  app= new KApplication(argc, argv, "ksnapshot");

  formatMngr= new FormatManager();

  // Create top level window
  toplevel= new KSnapShot();
  app->setMainWidget(toplevel);
  toplevel->show();
  app->exec();
}

