/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-2002
 * (c) Matthias Ettrich 2000
 * (c) Aaron J. Seigo 2002
 *
 * Released under the LGPL see file LICENSE for details.
 */


#include <klocale.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kimagefilepreview.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kguiitem.h>
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
#include <kpushbutton.h>

#include <qcursor.h>
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
    filename = conf->readEntry( "filename", QDir::currentDirPath()+"/"+i18n("snapshot")+"1.png" );

    // Make sure the name is not already being used
    QFileInfo fi( filename );
    while(fi.exists()) {
	autoincFilename();
	fi.setFile( filename );
    }

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);
    helpButton->setPopup(helpMenu->menu());

    helpButton->setGuiItem (KGuiItem(i18n("&Help"), "help" ) );
    closeButton->setGuiItem (KGuiItem(i18n("&Quit"), "exit" ) );
    
    KAccel* accel = new KAccel(this);
    accel->insert(KStdAccel::Quit, kapp, SLOT(quit()));
    accel->insert(KStdAccel::Save, this, SLOT(slotSave()));
    accel->insert(KStdAccel::Print, this, SLOT(slotPrint()));
    accel->insert(KStdAccel::New, this, SLOT(slotGrab()));

    saveButton->setFocus();
}

KSnapshot::~KSnapshot()
{
}

bool KSnapshot::save( const QString &filename )
{
    if ( QFileInfo(filename).exists() ) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" ).arg(filename);
        if (KMessageBox::Yes != KMessageBox::warningYesNoCancel( this, text, title ) ) 
        {
            QApplication::restoreOverrideCursor();
            return false;
        }
    }

    QCString type( KImageIO::type(filename).ascii() );

    if ( snapshot.save( filename, type ) ) {
	QApplication::restoreOverrideCursor();
	return true;
    }
    else {
	kdWarning() << "KSnapshot was unable to save the snapshot" << endl;

	QApplication::restoreOverrideCursor();
	QString caption = i18n("Error: Unable to save image");
	QString text = i18n("KSnapshot was unable to save the image to\n%1.")
	               .arg(filename);
	KMessageBox::error(this, text, caption);
    }

    return false;
}

void KSnapshot::slotSave()
{
    if ( save(filename) )
	autoincFilename();
}

void KSnapshot::slotSaveAs()
{
    QStringList mimetypes = KImageIO::mimeTypes( KImageIO::Reading );
    KFileDialog dlg( QString::null, mimetypes.join(" "), this, "filedialog", true);

    dlg.setSelection( filename );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );

    KImageFilePreview *ip = new KImageFilePreview( &dlg );
    dlg.setPreviewWidget( ip );

    if ( !dlg.exec() )
	return;

    QString name = dlg.selectedFile();
    if ( name.isNull() )
	return;

    if ( save(name) ) {
	filename = name;
	autoincFilename();
    }
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

    qApp->processEvents();

    if (printer.setup(this))
    {
	qApp->processEvents();

        QPainter painter(&printer);
        QPaintDeviceMetrics metrics(painter.device());

	int w = snapshot.width();
	int h = snapshot.height();
	bool scale = false;
	if ( w > metrics.width() )
	    scale = true;
	else if ( h > metrics.height() )
	    scale = true;

	if ( scale ) {
	    QImage img = snapshot.convertToImage();
	    qApp->processEvents();

	    img = img.smoothScale( metrics.width(), metrics.height(), QImage::ScaleMin );
	    qApp->processEvents();

	    int x = (metrics.width()-img.width())/2;
	    int y = (metrics.height()-img.height())/2;
	    painter.drawImage( x, y, img);
	}
	else {
	    int x = (metrics.width()-snapshot.width())/2;
	    int y = (metrics.height()-snapshot.height())/2;
	    painter.drawPixmap( x, y, snapshot );
	}
    }

    qApp->processEvents();
}

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    conf->writeEntry("delay",delaySpin->value());
    conf->writeEntry("onlyWindow",onlyWindow->isChecked());
    conf->writeEntry("filename",filename);
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
    int start= numSearch.search(name, 0);
    if (start != -1) {
        // It has a number, increment it
        int len = numSearch.matchedLength();
	QString numAsStr= name.mid(start, len);
	name.replace(start, len, QString::number(numAsStr.toInt() + 1));
    }
    else {
        // no number
        start = name.findRev('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            name.insert(start, '1');
        }
        else {
            // no extension, just tack it on to the end
            name += '1';
        }
    }

    //Rebuilt the path
    filename = path + '/' + name;
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

	int x, y;
	unsigned int w, h;
	unsigned int border;
	unsigned int depth;
	XGetGeometry( qt_xdisplay(), child, &root, &x, &y,
		      &w, &h, &border, &depth );

	snapshot = QPixmap::grabWindow( qt_xrootwin(), x, y, w, h );
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

void KSnapshot::slotMovePointer(int x, int y)
{
    QCursor::setPos( x, y );
}

void KSnapshot::exit()
{
    this->reject();
}
#include "ksnapshot.moc"
