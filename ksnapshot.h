// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H

#include <kapp.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <qwidget.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qpushbt.h>
#include <qlabel.h>
#include <qlined.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qchkbox.h>
#include <kbutton.h>
#include "preview.h"

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
  void filenameChangedSlot(const char *);
  void formatChangedSlot(const char *);
  void browsePressedSlot();
  void delayChangedSlot(const char *text);
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
  KButton *previewButton;
  QLabel *filenameLabel;
  QLineEdit *filenameEdit;
  QPushButton *browseButton;
  QComboBox *formatBox;
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
  QString format_;
  Window child;
};

#endif // KSNAPSHOT_H

