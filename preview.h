// -*- c++ -*-

#ifndef PREVIEW_H
#define PREVIEW_H

#include <qpushbutton.h>

class Preview : public QPushButton {
  Q_OBJECT
public:
  Preview();
  ~Preview();
  void closeEvent(QCloseEvent *e);

signals:
  void pressed();
};

#endif // PREVIEW_H
