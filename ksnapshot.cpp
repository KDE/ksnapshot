/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-2002
 * (c) Matthias Ettrich 2000
 * (c) Aaron J. Seigo 2002
 * (c) Nadeem Hasan 2003
 * (c) Bernd Brandstetter 2004
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
#include <kprinter.h>
#include <kio/netaccess.h>
#include <ksavefile.h>
#include <ktempfile.h>

#include <qbitmap.h>
#include <qdragobject.h>
#include <qimage.h>
#include <qclipboard.h>
#include <qvbox.h>

#include <kaccel.h>
#include <knotifyclient.h>
#include <khelpmenu.h>
#include <kpopupmenu.h>
#include <kpushbutton.h>
#include <kstartupinfo.h>

#include <qcursor.h>
#include <qregexp.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qwhatsthis.h>

#include <stdlib.h>

#include "ksnapshot.h"
#include "regiongrabber.h"
#include "windowgrabber.h"
#include "ksnapshotwidget.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <config.h>

#include <kglobal.h>

#define kApp KApplication::kApplication()

KSnapshot::KSnapshot(QWidget *parent, const char *name, bool grabCurrent)
  : DCOPObject("interface"), 
    KDialogBase(parent, name, true, QString::null, Help|User1, User1, 
    true, KStdGuiItem::quit() )
{
    grabber = new QWidget( 0, 0, WStyle_Customize | WX11BypassWM );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    KStartupInfo::appStarted();

    QVBox *vbox = makeVBoxMainWidget();
    mainWidget = new KSnapshotWidget( vbox, "mainWidget" );

    connect(mainWidget, SIGNAL(startImageDrag()), SLOT(slotDragSnapshot()));

    connect( mainWidget, SIGNAL( newClicked() ), SLOT( slotGrab() ) );
    connect( mainWidget, SIGNAL( saveClicked() ), SLOT( slotSaveAs() ) );
    connect( mainWidget, SIGNAL( printClicked() ), SLOT( slotPrint() ) );
    connect( mainWidget, SIGNAL( copyClicked() ), SLOT( slotCopy() ) );

    grabber->show();
    grabber->grabMouse( waitCursor );

    if ( !grabCurrent )
	snapshot = QPixmap::grabWindow( qt_xrootwin() );
    else {
	mainWidget->setMode( WindowUnderCursor );
	mainWidget->setIncludeDecorations( true );
	performGrab();
    }

    updatePreview();
    grabber->releaseMouse();
    grabber->hide();

    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    mainWidget->setDelay(conf->readNumEntry("delay",0));
    mainWidget->setMode( conf->readNumEntry( "mode", 0 ) );
    mainWidget->setIncludeDecorations(conf->readBoolEntry("includeDecorations",true));
    filename = KURL::fromPathOrURL( conf->readPathEntry( "filename", QDir::currentDirPath()+"/"+i18n("snapshot")+"1.png" ));

    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, false, this )) {
	autoincFilename();
    }

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    connect( &updateTimer, SIGNAL( timeout() ), this, SLOT(  updatePreview() ) );
    QTimer::singleShot( 0, this, SLOT( updateCaption() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);

    QPushButton *helpButton = actionButton( Help );
    helpButton->setPopup(helpMenu->menu());

    KAccel* accel = new KAccel(this);
    accel->insert(KStdAccel::Quit, kapp, SLOT(quit()));
    accel->insert( "QuickSave", i18n("Quick Save Snapshot &As..."),
		   i18n("Save the snapshot to the file specified by the user without showing the file dialog."),
		   CTRL+SHIFT+Key_S, this, SLOT(slotSave()));
    accel->insert(KStdAccel::Save, this, SLOT(slotSaveAs()));
//    accel->insert(KShortcut(CTRL+Key_A), this, SLOT(slotSaveAs()));
    accel->insert( "SaveAs", i18n("Save Snapshot &As..."),
		   i18n("Save the snapshot to the file specified by the user."),
		   CTRL+Key_A, this, SLOT(slotSaveAs()));
    accel->insert(KStdAccel::Print, this, SLOT(slotPrint()));
    accel->insert(KStdAccel::New, this, SLOT(slotGrab()));
    accel->insert(KStdAccel::Copy, this, SLOT(slotCopy()));

    accel->insert( "Quit2", Key_Q, this, SLOT(slotSave()));
    accel->insert( "Save2", Key_S, this, SLOT(slotSaveAs()));
    accel->insert( "Print2", Key_P, this, SLOT(slotPrint()));
    accel->insert( "New2", Key_N, this, SLOT(slotGrab()));
    accel->insert( "New3", Key_Space, this, SLOT(slotGrab()));

    setEscapeButton( User1 );
    connect( this, SIGNAL( user1Clicked() ), SLOT( reject() ) );

    mainWidget->btnNew->setFocus();
}

KSnapshot::~KSnapshot()
{
}

void KSnapshot::resizeEvent( QResizeEvent *event)
{
	if( !updateTimer.isActive() )
		updateTimer.start(200, true);
	else	
		updateTimer.changeInterval(200);
}

bool KSnapshot::save( const QString &filename )
{
    return save( KURL::fromPathOrURL( filename ));
}

bool KSnapshot::save( const KURL& url )
{
    if ( KIO::NetAccess::exists( url, false, this ) ) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" ).arg(url.prettyURL());
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel( this, text, title, i18n("Overwrite") ) ) 
        {
            return false;
        }
    }

    QString type( KImageIO::type(url.path()) );
    if ( type.isNull() )
	type = "PNG";

    bool ok = false;

    if ( url.isLocalFile() ) {
	KSaveFile saveFile( url.path() );
	if ( saveFile.status() == 0 ) {
	    if ( snapshot.save( saveFile.file(), type.latin1() ) )
		ok = saveFile.close();
	}
    }
    else {
	KTempFile tmpFile;
        tmpFile.setAutoDelete( true );
	if ( tmpFile.status() == 0 ) {
	    if ( snapshot.save( tmpFile.file(), type.latin1() ) ) {
		if ( tmpFile.close() )
		    ok = KIO::NetAccess::upload( tmpFile.name(), url, this );
	    }
	}
    }

    QApplication::restoreOverrideCursor();
    if ( !ok ) {
	kdWarning() << "KSnapshot was unable to save the snapshot" << endl;

	QString caption = i18n("Unable to save image");
	QString text = i18n("KSnapshot was unable to save the image to\n%1.")
	               .arg(url.prettyURL());
	KMessageBox::error(this, text, caption);
    }

    return ok;
}

void KSnapshot::slotSave()
{
    if ( save(filename) ) {
        modified = false;
        autoincFilename();
    }
}

void KSnapshot::slotSaveAs()
{
    QStringList mimetypes = KImageIO::mimeTypes( KImageIO::Writing );
    KFileDialog dlg( filename.url(), mimetypes.join(" "), this, "filedialog", true);

    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );

    KImageFilePreview *ip = new KImageFilePreview( &dlg );
    dlg.setPreviewWidget( ip );

    if ( !dlg.exec() )
        return;

    KURL url = dlg.selectedURL();
    if ( !url.isValid() )
        return;

    if ( save(url) ) {
        filename = url;
        modified = false;
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
    drobj->setPixmap(mainWidget->preview());
    drobj->dragCopy();
}

void KSnapshot::slotGrab()
{
    hide();

    if ( mainWidget->delay() && mainWidget->mode() != Region )
	grabTimer.start( mainWidget->delay() * 1000, true );
    else {
	if ( mainWidget->mode() == Region ) {
	    rgnGrab = new RegionGrabber();
	    connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
		     SLOT( slotRegionGrabbed( const QPixmap & ) ) );
	}
	else {
	    grabber->show();
	    grabber->grabMouse( crossCursor );
	}
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

    if (printer.setup(this, i18n("Print Screenshot")))
    {
	qApp->processEvents();

        QPainter painter(&printer);
        QPaintDeviceMetrics metrics(painter.device());

	float w = snapshot.width();
	float dw = w - metrics.width();
	float h = snapshot.height();
	float dh = h - metrics.height();
	bool scale = false;

	if ( (dw > 0.0) || (dh > 0.0) )
	    scale = true;

	if ( scale ) {

	    QImage img = snapshot.convertToImage();
	    qApp->processEvents();

	    float newh, neww;
	    if ( dw > dh ) {
		neww = w-dw;
		newh = neww/w*h;
	    }
	    else {
		newh = h-dh;
		neww = newh/h*w;
	    }

	    img = img.smoothScale( int(neww), int(newh), QImage::ScaleMin );
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

void KSnapshot::slotRegionGrabbed( const QPixmap &pix )
{
  if ( !pix.isNull() )
  {
    snapshot = pix;
    updatePreview();
    modified = true;
    updateCaption();
  }

  delete rgnGrab;
  QApplication::restoreOverrideCursor();
  show();
}

void KSnapshot::slotWindowGrabbed( const QPixmap &pix )
{
    if ( !pix.isNull() )
    {
        snapshot = pix;
        updatePreview();
        modified = true;
        updateCaption();
    }

    QApplication::restoreOverrideCursor();
    show();
}

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    conf->writeEntry("delay",mainWidget->delay());
    conf->writeEntry("mode",mainWidget->mode());
    conf->writeEntry("includeDecorations",mainWidget->includeDecorations());
    KURL url = filename;
    url.setPass( QString::null );
    conf->writePathEntry("filename",url.url());
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
    QString name= filename.fileName();

    // If the name contains a number then increment it
    QRegExp numSearch("[0-9]+");

    // Does it have a number?
    int start = numSearch.search(name);
    if (start != -1) {
        // It has a number, increment it
        int len = numSearch.matchedLength();
	QString numAsStr= name.mid(start, len);
	QString number = QString::number(numAsStr.toInt() + 1);
	number = number.rightJustify( len, '0');
	name.replace(start, len, number );
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

    //Rebuild the path 
    KURL newURL = filename;
    newURL.setFileName( name );
    setURL( newURL.url() );
}

void KSnapshot::updatePreview()
{
    mainWidget->setPreview( snapshot );
}

void KSnapshot::grabTimerDone()
{
    if ( mainWidget->mode() == Region ) {
        rgnGrab = new RegionGrabber();
        connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
            SLOT( slotRegionGrabbed( const QPixmap & ) ) );
    }
    else {
	performGrab();
    }
    KNotifyClient::beep(i18n("The screen has been successfully grabbed."));
}

void KSnapshot::performGrab()
{
    grabber->releaseMouse();
    grabber->hide();
    grabTimer.stop();
    if ( mainWidget->mode() == ChildWindow ) {
	WindowGrabber wndGrab;
	connect( &wndGrab, SIGNAL( windowGrabbed( const QPixmap & ) ),
	    SLOT( slotWindowGrabbed( const QPixmap & ) ) );
	wndGrab.exec();
	}
    else if ( mainWidget->mode() == WindowUnderCursor ) {
	snapshot = WindowGrabber::grabCurrent( mainWidget->includeDecorations() );
    }
    else {
	snapshot = QPixmap::grabWindow( qt_xrootwin() );
    }
    updatePreview();
    QApplication::restoreOverrideCursor();
    modified = true;
    updateCaption();
    show();
}

void KSnapshot::setTime(int newTime)
{
    mainWidget->setDelay(newTime);
}

int KSnapshot::timeout()
{
    return mainWidget->delay();
}

void KSnapshot::setURL( const QString &url )
{
    KURL newURL = KURL::fromPathOrURL( url );
    if ( newURL == filename )
	return;

    filename = newURL;
    updateCaption();
}

void KSnapshot::setGrabMode( int m )
{
    mainWidget->setMode( m );
}

int KSnapshot::grabMode()
{
    return mainWidget->mode();
}

void KSnapshot::updateCaption()
{
    setCaption( kApp->makeStdCaption( filename.fileName(), true, modified ) );
}

void KSnapshot::slotMovePointer(int x, int y)
{
    QCursor::setPos( x, y );
}

void KSnapshot::exit()
{
    reject();
}
#include "ksnapshot.moc"
