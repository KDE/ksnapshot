/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-2000
 * (c) Matthias Ettrich 2000
 *
 * Released under the LGPL see file LICENSE for details.
 */


#include <klocale.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <kapp.h>
#include <qimage.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <klineedit.h>
#include <knotifyclient.h>

#include <stdlib.h>

#include "ksnapshot.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

KSnapshot::KSnapshot(QWidget *parent, const char *name)
  : KSnapshotBase(parent, name)
  , DCOPObject("interface")
{

    grabber = new QWidget( 0, 0, WStyle_Customize | WX11BypassWM );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    grabber->show();
    grabber->grabMouse( waitCursor );
    snapshot = QPixmap::grabWindow( qt_xrootwin() );
    updatePreview();
    grabber->releaseMouse();
    grabber->hide();

    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    delaySpin->setValue(conf->readNumEntry("delay",0));
    onlyWindow->setChecked(conf->readBoolEntry("onlyWindow",true));

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    urlRequester->setURL( QDir::currentDirPath() + "/" + i18n("snapshot") + "1.png" );
    urlRequester->fileDialog()->setKeepLocation(true);

    // Make sure the name is not already being used
    QFileInfo fi( urlRequester->url());
    while(fi.exists()) {
	autoincFilename();
	fi.setFile( urlRequester->url() );
    }

    setTabOrder( PushButton3, urlRequester->lineEdit() );
    setTabOrder( urlRequester->lineEdit(), urlRequester->button() );
    setTabOrder( urlRequester->button(), delaySpin );
    urlRequester->lineEdit()->setFocus();
}

KSnapshot::~KSnapshot()
{
}


void KSnapshot::slotSave()
{
    QString text;
    QString caption(i18n("Error: Unable to save image"));
    QString buttonLabel(i18n("Dismiss"));

    QString overwriteCaption(i18n("Warning: This will overwrite an existing file"));
    QString overwriteMessage(i18n("Are you sure you want to overwrite the existing file named\n%1?"));
    QString overwriteButtonLabel(i18n("Overwrite"));
    QString cancelButtonLabel(i18n("Cancel"));

    QString saveErrorMessage(i18n("KSnapshot was unable to save the image to\n%1."));

    bool cancelled = false;

    QString filename = urlRequester->url();
    // Test to see if save will overwrite an existing file
    QFileInfo filenameInfo(filename);

    if (filenameInfo.exists()) {
	// Warn the user
	int choice= -1;

	text = overwriteMessage.arg(filename);
	choice= KMessageBox::warningYesNo(this, text, overwriteCaption, overwriteButtonLabel, cancelButtonLabel);

	// If the user chose to cancel
	if (choice != KMessageBox::Yes)
	    cancelled= true;
    }

    if (!cancelled) {
	QApplication::setOverrideCursor( waitCursor );
	// Cannot save (permissions error?)
	if ( !(snapshot.save(filename, KImageIO::type(filename).ascii() ) ) ) {
	    QApplication::restoreOverrideCursor();
	    kdWarning() << "KSnapshot was unable to save the snapshot" << endl;
	    QString caption = i18n("Error: Unable to save image");
	    QString text = i18n("KSnapshot was unable to save the image to\n%1.")
			   .arg(filename);
	    KMessageBox::error(this, text, caption);
	}
	QApplication::restoreOverrideCursor();
	autoincFilename();
    }
}

void KSnapshot::slotGrab()
{
    hide();
    if ( delaySpin->value() ) {
	grabTimer.start( delaySpin->value() * 1000, true );
    } else {
	grabber->show();
	grabber->grabMouse( crossCursor );
    }
}

void KSnapshot::slotHelp()
{
    kapp->invokeHelp();
}

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    conf->writeEntry("delay",delaySpin->value());
    conf->writeEntry("onlyWindow",onlyWindow->isChecked());
    e->accept();
}

bool KSnapshot::eventFilter( QObject* o, QEvent* e)
{
    if ( o == grabber && e->type() == QEvent::MouseButtonPress ) {
	QMouseEvent* me = (QMouseEvent*) e;
	if ( QWidget::mouseGrabber() != grabber )
	    return false;
	if ( me->button() == LeftButton )
	    performGrab();
    }
    return false;
}

void KSnapshot::autoincFilename()
{
    // Extract the filename from the path
    QFileInfo fi( urlRequester->url() );
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
	urlRequester->setURL( path );
    }
}

void KSnapshot::updatePreview()
{
    QImage img = snapshot.convertToImage();
    double r1 = ((double) snapshot.height() ) / snapshot.width();
    if ( r1 * imageLabel->width()  < imageLabel->height() )
	img = img.smoothScale( imageLabel->width(), (int) imageLabel->width() * r1 );
    else
	img = img.smoothScale( (int) (((double)imageLabel->height()) / r1) , (imageLabel->height() ) );

    QPixmap pm;
    pm.convertFromImage( img );
    imageLabel->setPixmap( pm );
}

void KSnapshot::grabTimerDone()
{
    performGrab();
    KNotifyClient::beep(i18n("The screen has been successfully grabbed."));
}

void KSnapshot::performGrab()
{
    grabber->releaseMouse();
    grabber->hide();
    grabTimer.stop();
    if ( onlyWindow->isChecked() ) {
	Window root;
	Window child;
	uint mask;
	int rootX, rootY, winX, winY;
	XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		       &rootX, &rootY, &winX, &winY,
		      &mask);
	snapshot = QPixmap::grabWindow( child );
    } else {
	snapshot = QPixmap::grabWindow( qt_xrootwin() );
    }
    updatePreview();
    show();
}

void KSnapshot::setTime(int newTime)
{
        delaySpin->setValue(newTime);
}

void KSnapshot::setURL(QString newURL)
{
        urlRequester->setURL( newURL );
}

void KSnapshot::setGrabPointer(bool grab)
{
        onlyWindow->setChecked( grab );
}

void KSnapshot::exit()
{
        this->reject();
}
#include "ksnapshot.moc"
