#include "preview.h"

Preview::Preview()
  : QPushButton(QString::null, 0)
{

}

Preview::~Preview()
{

}

void Preview::closeEvent(QCloseEvent *)
{
  emit clicked();
}
