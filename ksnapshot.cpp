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

#include <qbitmap.h>
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
#include <kstartupinfo.h>

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

#include <config.h>

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif

#include <kglobal.h>

#define kApp KApplication::kApplication()

KSnapshot::KSnapshot(QWidget *parent, const char *name)
  : KSnapshotBase(parent, name)
  , DCOPObject("interface")
{
    imageLabel->setAlignment(AlignHCenter | AlignVCenter);
    connect(imageLabel, SIGNAL(startDrag()), this, SLOT(slotDragSnapshot()));

    grabber = new QWidget( 0, 0, WStyle_Customize | WX11BypassWM );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    KStartupInfo::appStarted();

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
    int tmp1, tmp2;
    //Check whether the extension is available
    haveXShape = XShapeQueryExtension( qt_xdisplay(), &tmp1, &tmp2 );
#endif

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
    includeDecorations->setChecked(conf->readBoolEntry("includeDecorations",true));
    filename = conf->readPathEntry( "filename", QDir::currentDirPath()+"/"+i18n("snapshot")+"1.png" );

    // Make sure the name is not already being used
    QFileInfo fi( filename );
    while(fi.exists()) {
	autoincFilename();
	fi.setFile( filename );
    }

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    QTimer::singleShot( 0, this, SLOT( updateCaption() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);
    helpButton->setPopup(helpMenu->menu());

    helpButton->setGuiItem (KGuiItem(i18n("&Help"), "help" ) );
    closeButton->setGuiItem (KGuiItem(i18n("&Quit"), "exit" ) );

    KAccel* accel = new KAccel(this);
    accel->insert(KStdAccel::Quit, kapp, SLOT(quit()));
    accel->insert(KStdAccel::Save, this, SLOT(slotSave()));
//    accel->insert(KShortcut(CTRL+Key_A), this, SLOT(slotSaveAs()));
    accel->insert( "SaveAs", i18n("Save Snapshot &As..."),
		   i18n("Save the snapshot to the file specfied by the user."),
		   CTRL+Key_A, this, SLOT(slotSaveAs()));
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
            return false;
        }
    }

    QString type( KImageIO::type(filename) );
    if ( type.isNull() )
	type = "PNG";

    if ( snapshot.save( filename, type.ascii() ) ) {
	QApplication::restoreOverrideCursor();
	return true;
    }
    else {
	kdWarning() << "KSnapshot was unable to save the snapshot" << endl;

	QApplication::restoreOverrideCursor();
	QString caption = i18n("Unable to save image");
	QString text = i18n("KSnapshot was unable to save the image to\n%1.")
	               .arg(filename);
	KMessageBox::error(this, text, caption);
    }

    return false;
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

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    conf->writeEntry("delay",delaySpin->value());
    conf->writeEntry("onlyWindow",onlyWindow->isChecked());
    conf->writeEntry("includeDecorations",includeDecorations->isChecked());
    conf->writePathEntry("filename",filename);
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
    setURL( path + '/' + name );
}

void KSnapshot::updatePreview()
{
    QImage img = snapshot.convertToImage();
    double r1 = ((double) snapshot.height() ) / snapshot.width();
    if ( r1 * imageLabel->width()  < imageLabel->height() )
	img = img.smoothScale( int( imageLabel->width()), int( imageLabel->width() * r1 ));
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
    if ( onlyWindow->isChecked() ) {
	Window root;
	Window child;
	uint mask;
	int rootX, rootY, winX, winY;
	XQueryPointer( qt_xdisplay(), qt_xrootwin(), &root, &child,
		       &rootX, &rootY, &winX, &winY,
		      &mask);

	if( !includeDecorations->isChecked()) {
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
    updatePreview();
    QApplication::restoreOverrideCursor();
    modified = true;
    updateCaption();
    show();
}

void KSnapshot::setTime(int newTime)
{
    delaySpin->setValue(newTime);
}

void KSnapshot::setURL( const QString &url )
{
    if ( url == filename )
	return;

    filename = url;
    updateCaption();
}

void KSnapshot::updateCaption()
{
    QFileInfo fi( filename );
    setCaption( kApp->makeStdCaption( fi.fileName(), true, modified ) );
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
