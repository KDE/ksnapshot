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
#include <qimage.h>
#include <qclipboard.h>
#include <kstdaccel.h>
#include <QImageWriter>
#include <QPixmap>
#include <QCloseEvent>
#include <QEvent>
#include <QResizeEvent>
#include <QMouseEvent>


#include <knotification.h>
#include <khelpmenu.h>
#include <kmenu.h>
#include <kpushbutton.h>
#include <kstartupinfo.h>
#include <kvbox.h>

#include <qcursor.h>
#include <qregexp.h>
#include <qpainter.h>

#include <stdlib.h>

#include "ksnapshot.h"
#include "ksnapshotwidget.h"
#include "regiongrabber.h"
#include "windowgrabber.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <config.h>

#include <kglobal.h>
#include <QX11Info>

#define kApp KApplication::kApplication()

KSnapshot::KSnapshot(QWidget *parent, bool grabCurrent)
  : DCOPObject("interface"), 
    KDialog(parent)
{
    setModal( true );
    enableButtonSeparator( true );
    setDefaultButton( User1 );
    setButtonMask( Help|User1, KStdGuiItem::quit() );
    grabber = new QWidget( 0, Qt::WStyle_Customize | Qt::WX11BypassWM );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    KStartupInfo::appStarted();

    KVBox *vbox = new KVBox( this );
    vbox->setSpacing( spacingHint() );
    setMainWidget( vbox );

    mainWidget = new KSnapshotWidget( vbox, "mainWidget" );

    connect(mainWidget, SIGNAL(startImageDrag()), SLOT(slotDragSnapshot()));

    connect( mainWidget, SIGNAL( newClicked() ), SLOT( slotGrab() ) );
    connect( mainWidget, SIGNAL( saveClicked() ), SLOT( slotSaveAs() ) );
    connect( mainWidget, SIGNAL( printClicked() ), SLOT( slotPrint() ) );
    connect( mainWidget, SIGNAL( copyClicked() ), SLOT( slotCopy() ) );

    grabber->show();
    grabber->grabMouse( Qt::waitCursor );

    if ( !grabCurrent )
	snapshot = QPixmap::grabWindow( QX11Info::appRootWindow() );
    else {
	mainWidget->setMode( WindowUnderCursor );
	mainWidget->setIncludeDecorations( true );
	performGrab();
    }

    grabber->releaseMouse();
    grabber->hide();

    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    mainWidget->setDelay( conf->readEntry("delay", 0) );
    mainWidget->setMode( conf->readEntry("mode", 0) );
    mainWidget->setIncludeDecorations(conf->readEntry("includeDecorations",true));
    filename = KUrl::fromPathOrURL( conf->readPathEntry( "filename", QDir::currentPath()+"/"+i18n("snapshot")+"1.png" ));

    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, false, this )) {
	autoincFilename();
    }

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    connect( &updateTimer, SIGNAL( timeout() ), this, SLOT(  updatePreview() ) );
    QTimer::singleShot( 0, this, SLOT( updateCaption() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);

    QPushButton *helpButton = actionButton( Help );
    helpButton->setMenu(helpMenu->menu());

#warning Porting needed
#if 0
    KAccel* accel = new KAccel(this);
    accel->insert(KStdAccel::Quit, kapp, SLOT(quit()));
    accel->insert( "QuickSave", i18n("Quick Save Snapshot &As..."),
		   i18n("Save the snapshot to the file specified by the user without showing the file dialog."),
		   Qt::CTRL+Qt::SHIFT+Qt::Key_S, this, SLOT(slotSave()));
    accel->insert(KStdAccel::Save, this, SLOT(slotSaveAs()));
//    accel->insert(KShortcut(CTRL+Key_A), this, SLOT(slotSaveAs()));
    accel->insert( "SaveAs", i18n("Save Snapshot &As..."),
		   i18n("Save the snapshot to the file specified by the user."),
		   Qt::CTRL+Qt::Key_A, this, SLOT(slotSaveAs()));
    accel->insert(KStdAccel::Print, this, SLOT(slotPrint()));
    accel->insert(KStdAccel::New, this, SLOT(slotGrab()));
    accel->insert(KStdAccel::Copy, this, SLOT(slotCopy()));

    accel->insert( "Quit2", Qt::Key_Q, this, SLOT(slotSave()));
    accel->insert( "Save2", Qt::Key_S, this, SLOT(slotSaveAs()));
    accel->insert( "Print2", Qt::Key_P, this, SLOT(slotPrint()));
    accel->insert( "New2", Qt::Key_N, this, SLOT(slotGrab()));
    accel->insert( "New3", Qt::Key_Space, this, SLOT(slotGrab()));
#endif

    setEscapeButton( User1 );
    connect( this, SIGNAL( user1Clicked() ), SLOT( reject() ) );

    mainWidget->btnNew->setFocus();
}

KSnapshot::~KSnapshot()
{
}

void KSnapshot::resizeEvent( QResizeEvent * )
{
	updateTimer.setSingleShot( !updateTimer.isActive() );
	updateTimer.start( 200 );
}

bool KSnapshot::save( const QString &filename )
{
    return save( KUrl::fromPathOrURL( filename ));
}

bool KSnapshot::save( const KUrl& url )
{
    if ( KIO::NetAccess::exists( url, false, this ) ) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" , url.prettyURL());
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel( this, text, title, i18n("Overwrite") ) ) 
        {
            return false;
        }
    }

    QByteArray type = "PNG";
    QString mime = KMimeType::findByURL( url.fileName(), 0, url.isLocalFile(), true )->name();
    QStringList types = KImageIO::typeForMime(mime);
    if ( !types.isEmpty() )
        type = types.first().toLatin1();

    bool ok = false;

    if ( url.isLocalFile() ) {
	KSaveFile saveFile( url.path() );
	if ( saveFile.status() == 0 ) {
	    if ( snapshot.save( saveFile.file(), type ) )
		ok = saveFile.close();
	}
    }
    else {
	KTempFile tmpFile;
        tmpFile.setAutoDelete( true );
	if ( tmpFile.status() == 0 ) {
	    if ( snapshot.save( tmpFile.file(), type ) ) {
		if ( tmpFile.close() )
		    ok = KIO::NetAccess::upload( tmpFile.name(), url, this );
	    }
	}
    }

    QApplication::restoreOverrideCursor();
    if ( !ok ) {
	kWarning() << "KSnapshot was unable to save the snapshot" << endl;

	QString caption = i18n("Unable to save image");
	QString text = i18n("KSnapshot was unable to save the image to\n%1.",
	                url.prettyURL());
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
    KFileDialog dlg( filename.url(), mimetypes.join(" "), this);

    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );

    KImageFilePreview *ip = new KImageFilePreview( &dlg );
    dlg.setPreviewWidget( ip );

    if ( !dlg.exec() )
        return;

    KUrl url = dlg.selectedURL();
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
    QDrag *drag = new QDrag(this);

    drag->setMimeData(new QMimeData);
    drag->mimeData()->setImageData(snapshot);
    drag->setPixmap(mainWidget->preview());
    drag->start();
}

void KSnapshot::slotGrab()
{
    hide();

    if ( mainWidget->delay() ) {
        grabTimer.setSingleShot( true );
	grabTimer.start( mainWidget->delay() * 1000 );
    }
    else {
	if ( mainWidget->mode() == Region ) {
	    rgnGrab = new RegionGrabber();
	    connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
		     SLOT( slotRegionGrabbed( const QPixmap & ) ) );
	}
	else {
	    grabber->show();
	    grabber->grabMouse( Qt::crossCursor );
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

	float w = snapshot.width();
	float dw = w - printer.width();
	float h = snapshot.height();
	float dh = h - printer.height();
	bool scale = false;

	if ( (dw > 0.0) || (dh > 0.0) )
	    scale = true;

	if ( scale ) {

	    QImage img = snapshot.toImage();
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

	    img = img.scaled( int(neww), int(newh), Qt::KeepAspectRatio, Qt::SmoothTransformation );
	    qApp->processEvents();

	    int x = (printer.width()-img.width())/2;
	    int y = (printer.height()-img.height())/2;

	    painter.drawImage( x, y, img);
	}
	else {
	    int x = (printer.width()-snapshot.width())/2;
	    int y = (printer.height()-snapshot.height())/2;
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
    KUrl url = filename;
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
	if ( me->button() == Qt::LeftButton )
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
    int start = numSearch.indexIn(name);
    if (start != -1) {
        // It has a number, increment it
        int len = numSearch.matchedLength();
	QString numAsStr= name.mid(start, len);
	name.replace(start, len, QString::number(numAsStr.toInt() + 1));
    }
    else {
        // no number
        start = name.lastIndexOf('.');
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
    KUrl newURL = filename;
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
    KNotification::beep(i18n("The screen has been successfully grabbed."));
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
	snapshot = QPixmap::grabWindow( QX11Info::appRootWindow() );
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
    KUrl newURL = KUrl::fromPathOrURL( url );
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
    setCaption( KInstance::makeStdCaption( filename.fileName(),
                KInstance::ModifiedCaption | KInstance::AppNameCaption) );
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
