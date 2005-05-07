/*
 * KSnapshot
 *
 * (c) Richard J. Moore 1997-2002
 * (c) Matthias Ettrich 2000
 * (c) Aaron J. Seigo 2002
 * (c) Nadeem Hasan 2003
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
#include "ksnapshotwidget.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <config.h>

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif

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

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
    int tmp1, tmp2;
    //Check whether the extension is available
    haveXShape = XShapeQueryExtension( qt_xdisplay(), &tmp1, &tmp2 );
#endif

    QVBox *vbox = makeVBoxMainWidget();
    mainWidget = new KSnapshotWidget( vbox, "mainWidget" );

    connect(mainWidget, SIGNAL(startImageDrag()), SLOT(slotDragSnapshot()));

    connect( mainWidget, SIGNAL( newClicked() ), SLOT( slotGrab() ) );
    connect( mainWidget, SIGNAL( saveClicked() ), SLOT( slotSaveAs() ) );
    connect( mainWidget, SIGNAL( printClicked() ), SLOT( slotPrint() ) );

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
        if (KMessageBox::Yes != KMessageBox::warningYesNoCancel( this, text, title ) ) 
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

    if ( mainWidget->delay() )
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

static
Window findRealWindow( Window w, int depth = 0 )
{
    if( depth > 5 )
	return None;
    static Atom wm_state = XInternAtom( qt_xdisplay(), "WM_STATE", False );
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char* prop;
    if( XGetWindowProperty( qt_xdisplay(), w, wm_state, 0, 0, False, AnyPropertyType,
	&type, &format, &nitems, &after, &prop ) == Success ) {
	if( prop != NULL )
	    XFree( prop );
	if( type != None )
	    return w;
    }
    Window root, parent;
    Window* children;
    unsigned int nchildren;
    Window ret = None;
    if( XQueryTree( qt_xdisplay(), w, &root, &parent, &children, &nchildren ) != 0 ) {
	for( unsigned int i = 0;
	     i < nchildren && ret == None;
	     ++i )
	    ret = findRealWindow( children[ i ], depth + 1 );
	if( children != NULL )
	    XFree( children );
    }
    return ret;
}

void KSnapshot::performGrab()
{
    grabber->releaseMouse();
    grabber->hide();
    grabTimer.stop();
    XGrabServer( qt_xdisplay());
    if ( mainWidget->mode() == WindowUnderCursor ) {
	Window root;
	Window child;
	uint mask;
	int rootX, rootY, winX, winY;
	XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		       &rootX, &rootY, &winX, &winY,
		      &mask);
        if( child == None )
            child = qt_xrootwin();
	if( !mainWidget->includeDecorations()) {
	    Window real_child = findRealWindow( child );
	    if( real_child != None ) // test just in case
		child = real_child;
	}
	int x, y;
	unsigned int w, h;
	unsigned int border;
	unsigned int depth;
	XGetGeometry( qt_xdisplay(), child, &root, &x, &y,
		      &w, &h, &border, &depth );
        w += 2 * border;
        h += 2 * border;

	Window parent;
	Window* children;
	unsigned int nchildren;
	if( XQueryTree( qt_xdisplay(), child, &root, &parent,
	    &children, &nchildren ) != 0 ) {
	    if( children != NULL )
		XFree( children );
	    int newx, newy;
	    Window dummy;
	    if( XTranslateCoordinates( qt_xdisplay(), parent, qt_xrootwin(),
		x, y, &newx, &newy, &dummy )) {
		x = newx;
		y = newy;
	    }
	}

	XWindowAttributes rootAttributes;
	if ( XGetWindowAttributes( qt_xdisplay(), root, &rootAttributes ) ) {
	    if ( ( x + w ) > rootAttributes.width ) {
		// then the window is partly off the screen
		w = rootAttributes.width - x;
	    }
	    if ( ( y + h ) > rootAttributes.height ) {
		// then the window is partly off the screen
		h = rootAttributes.height - y;
	    }
	}

	snapshot = QPixmap::grabWindow( qt_xrootwin(), x, y, w, h );

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
	//No XShape - no work.
	if (haveXShape) {
	    QBitmap mask(w, h);
	    //As the first step, get the mask from XShape.
	    int count, order;
	    XRectangle* rects = XShapeGetRectangles( qt_xdisplay(), child,
	                                             ShapeBounding, &count, &order);
	    //The ShapeBounding region is the outermost shape of the window;
	    //ShapeBounding - ShapeClipping is defined to be the border.
	    //Since the border area is part of the window, we use bounding
	    // to limit our work region
	    if (rects) {
		//Create a QRegion from the rectangles describing the bounding mask.
		QRegion contents;
		for (int pos = 0; pos < count; pos++)
		    contents += QRegion(rects[pos].x, rects[pos].y,
		                        rects[pos].width, rects[pos].height);
		XFree(rects);

		//Create the bounding box.
		QRegion bbox(0, 0, snapshot.width(), snapshot.height());
                
                if( border > 0 ) {
                    contents.translate( border, border );
                    contents += QRegion( 0, 0, border, h );
                    contents += QRegion( 0, 0, w, border );
                    contents += QRegion( 0, h - border, w, border );
                    contents += QRegion( w - border, 0, border, h );
                }
                
		//Get the masked away area.
		QRegion maskedAway = bbox - contents;
		QMemArray<QRect> maskedAwayRects = maskedAway.rects();

		//Construct a bitmap mask from the rectangles
		QPainter p(&mask);
		p.fillRect(0, 0, w, h, Qt::color1);
		for (uint pos = 0; pos < maskedAwayRects.count(); pos++)
		    p.fillRect(maskedAwayRects[pos], Qt::color0);
		p.end();

		snapshot.setMask(mask);
	    }
	}
#endif
    }
    else {
	snapshot = QPixmap::grabWindow( qt_xrootwin() );
    }
    XUngrabServer( qt_xdisplay());
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
