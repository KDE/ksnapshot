/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-1999
 *
 * Released under the LGPL see file LICENSE for details.
 */

#include <qdir.h>
#include <qbttngrp.h>
#include <qcombo.h>
#include <qradiobt.h>
#include <qfiledlg.h>
#include <qmsgbox.h>
#include <qpainter.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtoolbutton.h>

#include <klocale.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <stdlib.h>

#include "ksnapshot.h"

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
  repeat_= 0;
  filename_= QDir::currentDirPath();

  filename_.append("/");

  filename_.append(i18n("snapshot"));
  filename_.append("01.png");

  // Make sure the name is not already being used
  QFileInfo fi(filename_);

  while(fi.exists()) {
    autoincFilename();
    fi.setFile(filename_);
  }

  previewWindow= 0;
  buildGui();

  delayEdit->setText("2");

  resize(400, 300);
}

KSnapShot::~KSnapShot()
{
  if (previewWindow != 0)
    previewWindow->close();
}

void KSnapShot::autoincFilename()
{
  // Extract the filename from the path
  QFileInfo fi(filename_);
  QString path= fi.dirPath();
  QString name= fi.fileName();

  // If the name contains a number then increment it
  QRegExp numSearch("[0-9]+");

  // Does it have a number?
  int len;
  int start= numSearch.match(name, 0, &len);
  if (start != -1) {
    // It has a number
    QString numAsStr= name.mid(start, len);
    int num= numAsStr.toInt();

    // Increment the number
    num++;
    QString newNum;
    newNum.setNum(num);
    name.replace(start, len, newNum);

    // Rebuild the path
    path.append("/");
    path.append(name);
    filename_= path;
  }
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

  QString about = i18n(
		     "<qt><center><h1>KSnapshot</h1>"
		     "<p>Press the `Grab' button, the window under "
		     "the mouse cursor will be grabbed after the "
		     "specified delay.</P>"
		     "<p><font size=3>KSnapshot is copyright Richard Moore (rich@kde.org) "
		     "and is released under the LGPL license.</font></p>"
		     "<p><small>Version: %1</small></p></center></qt>").arg(KSNAPVERSION);
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
  previewButton= new QToolButton(this);
  // Grab the root window to go inside
  performGrab( true );

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

  buttonsLayout->addSpacing(filenameLabel->width()+4);
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
  s.setNum(delay_);
  delayEdit->setText(s);
  delayEdit->setFixedHeight(delayLabel->height()+8);
  secondsLabel= new QLabel(i18n("seconds."), this);
  secondsLabel->setAlignment(AlignLeft);
  secondsLabel->setFixedHeight(delayLabel->height());
  secondsLabel->setMinimumWidth(secondsLabel->width());
  delayLayout->addWidget(delayLabel);
  delayLayout->addWidget(delayEdit, 2);
  delayLayout->addWidget(secondsLabel, 4);

  // start test
  repeatLabel= new QLabel(i18n("Repeat:"), this);
  repeatLabel->setAlignment(AlignCenter);
  repeatLabel->adjustSize();
  repeatLabel->setFixedHeight(delayLabel->height());
  repeatLabel->setFixedWidth(repeatLabel->width());
  repeatEdit= new QLineEdit(this);
  s.setNum(repeat_+1);
  repeatEdit->setText(s);
  repeatEdit->setFixedHeight(repeatLabel->height()+8);
  repeatEdit->setMinimumWidth(25);
  timesLabel= new QLabel(i18n("times."), this);
  timesLabel->setAlignment(AlignLeft);
  timesLabel->setFixedHeight(repeatLabel->height());
  timesLabel->setMinimumWidth(timesLabel->width());
  delayLayout->addWidget(repeatLabel);
  delayLayout->addWidget(repeatEdit, 4);
  delayLayout->addWidget(timesLabel, 4);
  // end test
    
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
  grabWindowCheck= new QCheckBox(i18n("Only grab the window containing the pointer"),
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
  connect(hideSelfCheck, SIGNAL(toggled(bool)), this, SLOT(hideSelfToggledSlot()));
  connect(autoRaiseCheck, SIGNAL(toggled(bool)), this, SLOT(autoRaiseToggledSlot()));
  connect(grabWindowCheck, SIGNAL(toggled(bool)), this, SLOT(grabWindowToggledSlot()));
  connect(delayEdit, SIGNAL(textChanged(const QString&)),
	  this, SLOT(delayChangedSlot(const QString&)));
  connect(repeatEdit, SIGNAL(textChanged(const QString &)),
	  this, SLOT(repeatChangedSlot(const QString&)));
  connect(filenameEdit, SIGNAL(textChanged(const QString&)), 
	  this, SLOT(filenameChangedSlot(const QString&)));
  connect(previewButton, SIGNAL(clicked()), this, SLOT(showPreviewSlot()));
}


void KSnapShot::grabPressedSlot()
{
  if (!grabbing_)
    startGrab();
}

void KSnapShot::browsePressedSlot()
{
  QString t;
  int p;

  t= filename_;
  p= t.findRev('/');

  if (p != -1)
    t.truncate(p);
  else
    t= QDir::currentDirPath();

  KURL url = KFileDialog::getSaveURL(t,KImageIO::pattern(KImageIO::Writing),this); 

  if( url.isEmpty() )
    return;

  if( !url.isLocalFile() )
  {
    KMessageBox::sorry( 0L, i18n( "Only local files are supported yet." ) );
    return;
  }
  
  filenameEdit->setText(url.path());
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
  }
}

void KSnapShot::internalTimerSlot()
{
    performGrab();
}

void KSnapShot::updatePreview()
{
  int w= previewButton->width();
  int h= previewButton->height();

  QPixmap preview;
  preview = snapshot_.convertToImage().smoothScale(w,h);  

  previewButton->setPixmap(preview);
}

void KSnapShot::performGrab(bool initial)
{
  if (grabDesktop_ || (child == qt_xrootwin()) || (child == 0)) {
    snapshot_= QPixmap::grabWindow(QApplication::desktop()->winId());
  }
  else if (grabWindow_) {
    snapshot_= QPixmap::grabWindow(child);
  }

  grabbing_= false;

  // If we're doing it lots of times...
  if (repeat_ > 0) {
    repeat_--; 
 
    saveSlot();
    startGrab();
  }
  else if (!initial) {
    QString s;
    s.setNum(repeat_+1);
    repeatEdit->setText(s);
    updatePreview();
    saveSlot();
    if (hidden) {
      show();
      hidden= false;
    }
  }
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

void KSnapShot::repeatChangedSlot(const QString& text)
{
  QString s;
  s= text;
  repeat_= s.toInt() - 1;
  if (repeat_ < 0) {
    repeat_= 0;
  }
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

  QString overwriteCaption(i18n("Warning: This will overwrite an existing file"));
  QString overwriteMessage(i18n("Are you sure you want to overwrite the existing file named\n%1?"));
  QString overwriteButtonLabel(i18n("Overwrite"));
  QString cancelButtonLabel(i18n("Cancel"));

  QString saveErrorMessage(i18n("KSnapshot was unable to save the image to\n%1."));

  bool cancelled= false;

  // Test to see if save will overwrite an existing file
  QFileInfo *filenameInfo= (QFileInfo *) new QFileInfo(filename_);
  CHECK_PTR(filenameInfo);

  if (filenameInfo->exists()) {
    // Warn the user
    int choice= -1;

    text = overwriteMessage.arg(filename_);
    choice= QMessageBox::warning(this, overwriteCaption, text, overwriteButtonLabel, cancelButtonLabel);

    // If the user chose to cancel
    if (choice != 0)
      cancelled= true;
  }
  
  if (!cancelled) {
    // Cannot save (permissions error?)
    if (!(snapshot_.save(filename_, KImageIO::type(filename_)))) {
      warning("KSnapshot was unable to save the snapshot");
      QString caption = i18n("Error: Unable to save image");
      QString text = i18n("KSnapshot was unable to save the image to\n%1.")
	.arg(filename_);
      KMessageBox::error(this, text, caption);
    }
    else if (autoincFilename_) {
      autoincFilename();
      filenameEdit->setText(filename_);
    }
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
