#include <kapp.h>
#include "ksnapshot.h"
#include <kimgio.h>

int main(int argc, char **argv)
{
  KApplication *app;
  KSnapShot *toplevel;

  app= new KApplication(argc, argv, "ksnapshot");

  kimgioRegister();

  // Create top level window
  toplevel= new KSnapShot();
  app->setMainWidget(toplevel);
  toplevel->show();
  app->exec();
}

