#include "preview.h"

Preview::Preview()
  : QPushButton()
{

}

Preview::~Preview()
{

}

void Preview::closeEvent(QCloseEvent *e)
{
  emit clicked();
}
