// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H

#include <kapp.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <qwidget.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include "preview.h"

class QToolButton;


class KSnapShot : public QWidget
{
  Q_OBJECT

public:
  KSnapShot(QWidget *parent= 0, const char *name= 0);
  ~KSnapShot();

  void startGrab();
  void performGrab();
  void resizeEvent(QResizeEvent *);

signals:
  void doneGrab();

public slots:
  void grabPressedSlot();
  void autoRaiseToggledSlot();
  void hideSelfToggledSlot();
  void grabWindowToggledSlot();
  void filenameChangedSlot(const QString&);
  void browsePressedSlot();
  void delayChangedSlot(const QString&);
  void helpSlot();
  void closeSlot();
  void showPreviewSlot();
  void hidePreviewSlot();
  void saveSlot();

protected slots:
  void timerFinishedSlot();

protected:
  // The widgets
  QLabel *titleLabel;
  QLabel *logoLabel;
  QHBoxLayout *titleLayout;
  QLabel *hintLabel;
  QVBoxLayout *infoLayout;
  QToolButton *previewButton;
  QLabel *filenameLabel;
  QLineEdit *filenameEdit;
  QPushButton *browseButton;
  QHBoxLayout *filenameLayout;
  QLabel *delayLabel;
  QLineEdit *delayEdit;
  QLabel *secondsLabel;
  QHBoxLayout *delayLayout;
  QVBoxLayout *parametersLayout;
  QCheckBox *autoRaiseCheck;
  QCheckBox *hideSelfCheck;
  QCheckBox *grabWindowCheck;
  QVBoxLayout *checkLayout;
  QGridLayout *mainLayout;
  QBoxLayout *buttonLayout;

  QPushButton *helpButton;
  QPushButton *saveButton;
  QPushButton *grabButton;
  QPushButton *closeButton;
  Preview *previewWindow;

  void buildGui();
  void updatePreview();

private slots:
  void internalTimerSlot();

private:
  // Are we busy grabbing?
  bool grabbing_;
  bool hideSelf_;
  bool autoRaise_;
  bool grabWindow_;
  bool grabDesktop_;
  QPixmap snapshot_;
  int delay_;
  QTimer *timer_;
  QString filename_;
  Window child;
};

#endif // KSNAPSHOT_H

