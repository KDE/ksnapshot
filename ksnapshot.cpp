/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-2000
 * (c) Matthias Ettrich 2000
 * (c) Aaron J. Seigo 2002
 *
 * Released under the LGPL see file LICENSE for details.
 */


#include <klocale.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kprinter.h>
#include <qdragobject.h>
#include <qgroupbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qclipboard.h>
#include <kaccel.h>
#include <klineedit.h>
#include <knotifyclient.h>
#include <khelpmenu.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qwhatsthis.h>

#include <stdlib.h>

#include "ksnapshot.h"
#include "ksnapshotbase.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <kglobal.h>

KSnapshot::KSnapshot(QWidget *parent, const char *name)
  : KSnapshotBase(parent, name)
  , DCOPObject("interface")
{
    imageLabel->setAlignment(AlignHCenter | AlignVCenter);
    connect(imageLabel, SIGNAL(startDrag()), this, SLOT(slotDragSnapshot()));
 
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
    filename =  QDir::currentDirPath() + "/" + i18n("snapshot") + "1.png";

    // Make sure the name is not already being used
    QFileInfo fi( filename );
    while(fi.exists()) {
	autoincFilename();
	fi.setFile( filename );
    }

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);
    helpButton->setPopup(helpMenu->menu());

    helpButton->setIconSet( SmallIconSet( "help" ) );
    closeButton->setIconSet( SmallIconSet( "exit" ) );
    
    KAccel* accel = new KAccel(this);
    accel->insert(KStdAccel::Quit, kapp, SLOT(quit()));
    accel->insert(KStdAccel::Save, this, SLOT(slotSave()));
    accel->insert(KStdAccel::Print, this, SLOT(slotSave()));
    accel->insert(KStdAccel::New, this, SLOT(slotGrab()));

    saveButton->setFocus();
}

KSnapshot::~KSnapshot()
{
}

void KSnapshot::slotSave()
{
    QString saveTo = KFileDialog::getSaveFileName(filename, QString::null, this);
    if (!saveTo.isNull())
    {
        if ( !(snapshot.save(saveTo, KImageIO::type(filename).ascii() ) ) ) 
        {
            QApplication::restoreOverrideCursor();
            kdWarning() << "KSnapshot was unable to save the snapshot" << endl;
            QString caption = i18n("Error: Unable to save image");
            QString text = i18n("KSnapshot was unable to save the image to\n%1.")
                               .arg(filename);
            KMessageBox::error(this, text, caption);
        }
        QApplication::restoreOverrideCursor();
        filename = saveTo;
        autoincFilename();
    }

    return;
}

void KSnapshot::slotCopy()
{
  QClipboard *cb = QApplication::clipboard();
  cb->setPixmap( snapshot );
}

void KSnapshot::slotDragSnapshot()
{
    QDragObject *drobj = new QImageDrag(snapshot.convertToImage(), this);
    drobj->setPixmap(imageLabel->pixmap()->convertToImage());
    drobj->dragCopy();
}

void KSnapshot::slotGrab()
{
    hide();
    if ( delaySpin->value() ) 
	grabTimer.start( delaySpin->value() * 1000, true );
    else {
	grabber->show();
	grabber->grabMouse( crossCursor );
    }
}

void KSnapshot::slotPrint()
{
    KPrinter printer;
    if (snapshot.width() > snapshot.height())
        printer.setOrientation(KPrinter::Landscape);
    else
        printer.setOrientation(KPrinter::Portrait);

    if (printer.setup(this))
    {
        QPainter painter(&printer);
        QPaintDeviceMetrics metrics(painter.device());

        QImage img = snapshot.convertToImage().smoothScale(metrics.width(), metrics.height(), QImage::ScaleMin);
        painter.drawImage((metrics.width()-img.width())/2, (metrics.height()-img.height())/2, img);
    }
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
    QFileInfo fi( filename );
    QString path= fi.dirPath();
    QString name= fi.fileName();

    // If the name contains a number then increment it
    QRegExp numSearch("[0-9]+");

    // Does it have a number?
    int len;
    int start= numSearch.search(name, 0);
    len = numSearch.matchedLength();
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
        filename = path;
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
    } 
    else {
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
    filename = newURL;
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
