/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-1998
 *
 * Released under the LGPL see file LICENSE for details.
 */

#include <qdir.h>
#include "ksnapshot.h"
#include <qbttngrp.h>
#include <qcombo.h>
#include <qradiobt.h>
#include <qfiledlg.h>
#include <qwmatrix.h>
#include <qpainter.h>
#include <qmsgbox.h>
#include <qregexp.h>
#include <qstring.h>
#include "formats.h"
#include <klocale.h>

extern FormatManager *formatMngr;

KSnapShot::KSnapShot(QWidget *parent, const char *name)
  : QWidget(parent, name)
{
  // Initialise members
  grabbing_= false;
  hideSelf_= true;
  autoRaise_= true;
  grabDesktop_= true;
  grabWindow_= false;
  delay_= 0;
  timer_= 0;
  filename_= QDir::currentDirPath();

  filename_.append("/");

  format_= "GIF";
  filename_.append(i18n("snapshot"));
  filename_.append("01.gif");

  previewWindow= 0;
  buildGui();
  resize(480, 300);
}

KSnapShot::~KSnapShot()
{
  if (previewWindow != 0)
    previewWindow->close();
}

void KSnapShot::resizeEvent(QResizeEvent *)
{
  updatePreview();
}

void KSnapShot::buildGui()
{
  QString s;
  QPixmap previewPixmap;
  QFont titleFont("courier");
  
  titleFont.setPointSize(24);
  titleFont.setBold(true);

  // Create top level layout
  QVBoxLayout *topLevelLayout= new QVBoxLayout(this, 4);
  
  mainLayout= new QGridLayout(2, 2, 4);
  topLevelLayout->addLayout(mainLayout, 10);


  // -------
  // |**|  |
  // -------
  // |  |  |
  // -------
  infoLayout= new QVBoxLayout();
  mainLayout->addLayout(infoLayout, 0, 0);
  titleLayout= new QHBoxLayout();
  infoLayout->addLayout(titleLayout);

  titleLabel= new QLabel("KSnapshot", this);
  titleLabel->setFont(titleFont);
  titleLabel->setAlignment(AlignCenter);
  titleLabel->adjustSize();
  titleLabel->setMinimumSize(titleLabel->size());
  titleLayout->addWidget(titleLabel);

  QString about;
  about.sprintf(i18n(
		     "Press the `Grab' button, then click\n"
		     "on a window to grab it.\n\n"
		     "KSnapshot is copyright Richard Moore (rich@kde.org)\n"
		     "and is released under LGPL\n\n"
		     "Version: %s"), KSNAPVERSION);
  hintLabel= new QLabel(about, 
			this);
  hintLabel->setAlignment(AlignCenter);
  hintLabel->adjustSize();
  hintLabel->setMinimumSize(hintLabel->size());

  infoLayout->addWidget(hintLabel);

  // -------
  // |  |**|
  // -------
  // |  |  |
  // -------
  previewButton= new KButton(this);
  // Grab the root window to go inside
  performGrab();

  mainLayout->addWidget(previewButton, 0, 1);

  // -------
  // |  |  |
  // -------
  // |**|  |
  // -------
  parametersLayout= new QVBoxLayout(4);
  mainLayout->addLayout(parametersLayout, 1, 0);
  filenameLayout= new QHBoxLayout();
  parametersLayout->addStretch();
  parametersLayout->addLayout(filenameLayout, 0);

  filenameLabel= new QLabel(i18n("Filename:"), this);
  filenameLabel->setAlignment(AlignCenter);
  filenameLabel->adjustSize();
  filenameLabel->setFixedHeight(filenameLabel->height());
  filenameLabel->setFixedWidth(filenameLabel->width() * 1.5);
  filenameEdit= new QLineEdit(this);
  filenameEdit->setText(filename_);
  filenameEdit->setFixedHeight(filenameLabel->height()+8);
  filenameEdit->setMinimumWidth(70);
  filenameLayout->addWidget(filenameLabel);
  filenameLayout->addWidget(filenameEdit, 5);

  QHBoxLayout *buttonsLayout;
  buttonsLayout= new QHBoxLayout();
  parametersLayout->addLayout(buttonsLayout);

  formatBox= new QComboBox(this);
  formatBox->insertStrList(formatMngr->formats());
  formatBox->setCurrentItem(formatMngr->formats()->find(format_));
  formatBox->resize(formatBox->sizeHint());
  buttonsLayout->addSpacing(filenameLabel->width()+4);
  buttonsLayout->addWidget(formatBox, 1);
  buttonsLayout->addStretch();
  browseButton= new QPushButton(i18n("Browse..."), this);
  browseButton->setFixedHeight(filenameLabel->height()+8);
  browseButton->setMinimumWidth(filenameLabel->width());
  buttonsLayout->addWidget(browseButton, 2);

  delayLayout= new QHBoxLayout();
  parametersLayout->addLayout(delayLayout, 0);
  parametersLayout->addStretch();

  delayLabel= new QLabel(i18n("Delay:"), this);
  delayLabel->setAlignment(AlignCenter);
  delayLabel->adjustSize();
  delayLabel->setFixedHeight(delayLabel->height());
  delayLabel->setFixedWidth(filenameLabel->width());
  delayEdit= new QLineEdit(this);
  s.sprintf("%d", delay_);
  delayEdit->setText(s);
  delayEdit->setFixedHeight(delayLabel->height()+8);
  secondsLabel= new QLabel(i18n("seconds."), this);
  secondsLabel->setAlignment(AlignLeft);
  secondsLabel->setFixedHeight(delayLabel->height());
  secondsLabel->setMinimumWidth(secondsLabel->width());
  delayLayout->addWidget(delayLabel);
  delayLayout->addWidget(delayEdit, 2);
  delayLayout->addWidget(secondsLabel, 4);
    
  // -------
  // |  |  |
  // -------
  // |  |**|
  // -------
  checkLayout= new QVBoxLayout(4);
  mainLayout->addLayout(checkLayout, 1, 1);

  QGroupBox *checkGroup= new QGroupBox(i18n("Options"), this);

  QVBoxLayout *optionsBox= new QVBoxLayout(checkGroup, 16, 6);

  autoRaiseCheck= new QCheckBox(i18n("Auto raise"), checkGroup);
  hideSelfCheck= new QCheckBox(i18n("Hide KSnapshot window"),
			       checkGroup);
  grabWindowCheck= new QCheckBox(i18n("Only grab the window containing the cursor"),
				 checkGroup);

  autoRaiseCheck->setMinimumSize(autoRaiseCheck->sizeHint());
  hideSelfCheck->setMinimumSize(hideSelfCheck->sizeHint());
  grabWindowCheck->setMinimumSize(grabWindowCheck->sizeHint());

  if (grabWindow_) {
    grabWindowCheck->setChecked(true);
  }
  else if (grabDesktop_) {
    grabWindowCheck->setChecked(false);
  }

  autoRaiseCheck->setChecked(autoRaise_);
  hideSelfCheck->setChecked(hideSelf_);

  optionsBox->addWidget(autoRaiseCheck);
  optionsBox->addWidget(hideSelfCheck);
  optionsBox->addWidget(grabWindowCheck);
  optionsBox->activate();
  checkLayout->addWidget(checkGroup);

  mainLayout->setRowStretch(0, 3);
  mainLayout->setRowStretch(1, 1);
  mainLayout->setColStretch(0, 3);
  mainLayout->setColStretch(1, 2);

  // Buttons
  helpButton= new QPushButton(i18n("Help"), this);
  saveButton= new QPushButton(i18n("Save"), this);
  grabButton= new QPushButton(i18n("Grab"), this);
  closeButton= new QPushButton(i18n("Close"), this);

  buttonLayout= new QBoxLayout(QBoxLayout::RightToLeft);

  QFrame *frame= new QFrame();
  frame->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  frame->setFixedHeight(4);
  topLevelLayout->addWidget(frame);
  topLevelLayout->addLayout(buttonLayout, 0);

  closeButton->resize(closeButton->sizeHint());
  grabButton->resize(grabButton->sizeHint());
  saveButton->resize(saveButton->sizeHint());
  helpButton->resize(helpButton->sizeHint());
  closeButton->setFixedHeight(closeButton->height());
  grabButton->setFixedHeight(grabButton->height());
  saveButton->setFixedHeight(saveButton->height());
  helpButton->setFixedHeight(helpButton->height());


  buttonLayout->addWidget(closeButton);
  buttonLayout->addWidget(grabButton);
  buttonLayout->addWidget(saveButton);
  buttonLayout->addWidget(helpButton);

  topLevelLayout->activate();

  connect(helpButton, SIGNAL(clicked()), this, SLOT(helpSlot()));
  connect(grabButton, SIGNAL(clicked()), this, SLOT(grabPressedSlot()));
  connect(closeButton, SIGNAL(clicked()), this, SLOT(closeSlot()));
  connect(saveButton, SIGNAL(clicked()), this, SLOT(saveSlot()));
  connect(browseButton, SIGNAL(clicked()), this, SLOT(browsePressedSlot()));
  connect(formatBox, SIGNAL(activated(const QString&)), this, SLOT(formatChangedSlot(const QString&)));
  connect(hideSelfCheck, SIGNAL(toggled(bool)), this, SLOT(hideSelfToggledSlot()));
  connect(autoRaiseCheck, SIGNAL(toggled(bool)), this, SLOT(autoRaiseToggledSlot()));
  connect(grabWindowCheck, SIGNAL(toggled(bool)), this, SLOT(grabWindowToggledSlot()));
  connect(delayEdit, SIGNAL(textChanged(const QString&)), this, SLOT(delayChangedSlot(const QString&)));
  connect(filenameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(filenameChangedSlot(const QString&)));
  connect(previewButton, SIGNAL(clicked()), this, SLOT(showPreviewSlot()));
}


void KSnapShot::grabPressedSlot()
{
  if (!grabbing_)
    startGrab();
}

void KSnapShot::browsePressedSlot()
{
  QString s, t;
  QString f("*.");
  int p;

  f.append(formatMngr->suffix(format_));

  t= (const char *) filename_;
  p= t.findRev('/');

  if (p != -1)
    t.truncate(p);
  else
    t= QDir::currentDirPath();

  s= QFileDialog::getSaveFileName(t, f, this);

  if (!(s.isNull()))
    filenameEdit->setText(s);
}

void KSnapShot::startGrab()
{
  grabbing_= true;

  if (grabDesktop_ || (delay_ > 0)) {
    if (hideSelf_)
      hide();
  }

  if (delay_ > 0) {
    timer_= new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(timerFinishedSlot()));
    timer_->start(delay_*1000, true);
  }
  else {
    timerFinishedSlot();
  }
}

void KSnapShot::timerFinishedSlot()
{
  Display *display;
  Window root;
  uint mask;
  int rootX, rootY, winX, winY;

  QApplication::beep();

  delete timer_;

  // Do raise
  display= QApplication::desktop()->x11Display();
  root= DefaultRootWindow(display);

  XQueryPointer(display, root, &root, &child, 
		&rootX, &rootY, &winX, &winY,
		&mask);

  if (autoRaise_ && (!grabDesktop_)) {
    if ((child != 0) && (child != qt_xrootwin())) {
      XRaiseWindow(display, child);
    }

    XSync(display, false);

    timer_= new QTimer(this);
    connect(timer_, SIGNAL(timeout()), this, SLOT(internalTimerSlot()));
    timer_->start(250, true);
  }
  else {
    performGrab();
    show();
  }
}

void KSnapShot::internalTimerSlot()
{
    performGrab();
    show();
}

void KSnapShot::updatePreview()
{
  QPixmap *preview;
  QPainter p;
  QWMatrix matrix;
  double xf, yf;
  int w, h;

  w= previewButton->width()-10;
  h= previewButton->height()-10;

  matrix.reset();

  xf= ((double) w / snapshot_.width());
  yf= ((double) h / snapshot_.height());
  matrix.scale(xf, yf);

  preview= new QPixmap(w, h);

  p.begin(preview);
  p.setWorldMatrix(matrix);
  p.drawPixmap(5, 5, snapshot_);
  p.end();

  previewButton->setPixmap(*preview);
  delete preview;
}

void KSnapShot::performGrab()
{
  if (grabDesktop_ || (child == qt_xrootwin()) || (child == 0)) {
    snapshot_= QPixmap::grabWindow(QApplication::desktop()->winId());
  }
  else if (grabWindow_) {
    snapshot_= QPixmap::grabWindow(child);
  }

  updatePreview();
  grabbing_= false;
}

void KSnapShot::autoRaiseToggledSlot()
{
  autoRaise_= !autoRaise_;
}

void KSnapShot::hideSelfToggledSlot()
{
  hideSelf_= !hideSelf_;
}

void KSnapShot::grabWindowToggledSlot()
{
  grabWindow_= !grabWindow_;
  grabDesktop_= !grabDesktop_;
}

void KSnapShot::filenameChangedSlot(const QString& text)
{
  filename_= text;
}

void KSnapShot::delayChangedSlot(const QString& text)
{
  QString s;
  s= text;
  delay_= s.toInt();
}

void KSnapShot::formatChangedSlot(const QString& format)
{
  QFileInfo fi(filename_);

  QString s;

  s= fi.dirPath();
  s.append("/");
  s.append(fi.baseName());
  s.append(".");
  s.append(formatMngr->suffix(format));
  filenameEdit->setText(s);
  filename_= (const char *) s;
  format_= format;
}

void KSnapShot::helpSlot()
{
  kapp->invokeHTMLHelp("", "");
}

void KSnapShot::closeSlot()
{
  exit(0);
}

void KSnapShot::saveSlot()
{
  QString text;
  QString caption(i18n("Error: Unable to save image"));
  QString buttonLabel(i18n("Dismiss"));
			 
  if (!(snapshot_.save(filename_, format_))) {
    warning("KSnapshot was unable to save the snapshot");
    text.sprintf(i18n("KSnapshot was unable to save the image to\n%s."),
		 filename_.data());
    QMessageBox::warning(this, caption, text, buttonLabel);
  }
}

void KSnapShot::showPreviewSlot()
{
  if (previewWindow == 0) {
    previewWindow= new Preview();
    previewWindow->setCaption(i18n("KSnapshot preview"));
    previewWindow->setPixmap(snapshot_);
    previewWindow->resize(previewWindow->sizeHint());
    connect(previewWindow, SIGNAL(clicked()), this, SLOT(hidePreviewSlot()));
    previewWindow->show();
  }
  else {
    previewWindow->raise();
  }
}

void KSnapShot::hidePreviewSlot()
{
  if (previewWindow != 0)
    delete previewWindow;
  previewWindow= 0;
}
