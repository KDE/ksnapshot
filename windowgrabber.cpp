/*
  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or ( at your option ) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "windowgrabber.h"

#include <algorithm>

#include <kwindowinfo.h>
#include <kdebug.h>

#include <QBitmap>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QWheelEvent>

#include <X11/Xlib.h>
#include <config-ksnapshot.h>
#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
#include <X11/extensions/shape.h>
#endif
#include <QX11Info>

static
const int minSize = 8;

static
bool operator< ( const QRect& r1, const QRect& r2 )
{
    return r1.width() * r1.height() < r2.width() * r2.height();
}

// Recursively iterates over the window w and its children, thereby building
// a tree of window descriptors. Windows in non-viewable state or with height
// or width smaller than minSize will be ignored.
static
void getWindowsRecursive( std::vector<QRect>& windows, Window w,
			  int rx = 0, int ry = 0, int depth = 0 )
{
    XWindowAttributes atts;
    XGetWindowAttributes( QX11Info::display(), w, &atts );
    if ( atts.map_state == IsViewable &&
         atts.width >= minSize && atts.height >= minSize ) {
	int x = 0, y = 0;
	if ( depth ) {
	    x = atts.x + rx;
	    y = atts.y + ry;
	}

	QRect r( x, y, atts.width, atts.height );
	if ( std::find( windows.begin(), windows.end(), r ) == windows.end() ) {
	    windows.push_back( r );
	}

	Window root, parent;
	Window* children;
	unsigned int nchildren;

	if( XQueryTree( QX11Info::display(), w, &root, &parent, &children, &nchildren ) != 0 ) {
            for( unsigned int i = 0; i < nchildren; ++i ) {
                getWindowsRecursive( windows, children[ i ], x, y, depth + 1 );
	    }
            if( children != NULL )
		XFree( children );
	}
    }
    if ( depth == 0 )
	std::sort( windows.begin(), windows.end() );
}

static
Window findRealWindow( Window w, int depth = 0 )
{
    if( depth > 5 )
	return None;
    static Atom wm_state = XInternAtom( QX11Info::display(), "WM_STATE", False );
    Atom type;
    int format;
    unsigned long nitems, after;
    unsigned char* prop;
    if( XGetWindowProperty( QX11Info::display(), w, wm_state, 0, 0, False, AnyPropertyType,
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
    if( XQueryTree( QX11Info::display(), w, &root, &parent, &children, &nchildren ) != 0 ) {
	for( unsigned int i = 0;
	     i < nchildren && ret == None;
	     ++i )
	    ret = findRealWindow( children[ i ], depth + 1 );
	if( children != NULL )
	    XFree( children );
    }
    return ret;
}

static
Window windowUnderCursor( bool includeDecorations = true )
{
    Window root;
    Window child;
    uint mask;
    int rootX, rootY, winX, winY;
    XGrabServer( QX11Info::display() );
    XQueryPointer( QX11Info::display(), QX11Info::appRootWindow(), &root, &child,
		   &rootX, &rootY, &winX, &winY, &mask );
    if( child == None )
	child = QX11Info::appRootWindow();
    if( !includeDecorations ) {
	Window real_child = findRealWindow( child );
	if( real_child != None ) // test just in case
	    child = real_child;
    }
    return child;
}

static
QPixmap grabWindow( Window child, int x, int y, uint w, uint h, uint border,
		    QString *title=0, QString *windowClass=0 )
{
    QPixmap pm( QPixmap::grabWindow( QX11Info::appRootWindow(), x, y, w, h ) );

    KWindowInfo winInfo( findRealWindow(child), NET::WMVisibleName, NET::WM2WindowClass );
    if ( title )
	(*title) = winInfo.visibleName();
    if ( windowClass )
	(*windowClass) = winInfo.windowClassName();

#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
    int tmp1, tmp2;
    //Check whether the extension is available
    if ( XShapeQueryExtension( QX11Info::display(), &tmp1, &tmp2 ) ) {
	QBitmap mask( w, h );
	//As the first step, get the mask from XShape.
	int count, order;
	XRectangle* rects = XShapeGetRectangles( QX11Info::display(), child,
						 ShapeBounding, &count, &order );
	//The ShapeBounding region is the outermost shape of the window;
	//ShapeBounding - ShapeClipping is defined to be the border.
	//Since the border area is part of the window, we use bounding
	// to limit our work region
	if (rects) {
	    //Create a QRegion from the rectangles describing the bounding mask.
	    QRegion contents;
	    for ( int pos = 0; pos < count; pos++ )
		contents += QRegion( rects[pos].x, rects[pos].y,
				     rects[pos].width, rects[pos].height );
	    XFree( rects );

	    //Create the bounding box.
	    QRegion bbox( 0, 0, w, h );

	    if( border > 0 ) {
		contents.translate( border, border );
		contents += QRegion( 0, 0, border, h );
		contents += QRegion( 0, 0, w, border );
		contents += QRegion( 0, h - border, w, border );
		contents += QRegion( w - border, 0, border, h );
	    }

	    //Get the masked away area.
	    QRegion maskedAway = bbox - contents;
	    QVector<QRect> maskedAwayRects = maskedAway.rects();

	    //Construct a bitmap mask from the rectangles
	    QPainter p(&mask);
	    p.fillRect(0, 0, w, h, Qt::color1);
	    for (int pos = 0; pos < maskedAwayRects.count(); pos++)
		    p.fillRect(maskedAwayRects[pos], Qt::color0);
	    p.end();

	    pm.setMask(mask);
	}
    }
#endif

    return pm;
}

QString WindowGrabber::title;
QString WindowGrabber::windowClass;
QPoint WindowGrabber::windowPosition;

WindowGrabber::WindowGrabber()
: QDialog( 0, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint ),
  current( -1 ), yPos( -1 )
{
    setWindowModality( Qt::WindowModal );
    Window root;
    int y, x;
    uint w, h, border, depth;
    XGrabServer( QX11Info::display() );
    Window child = windowUnderCursor();
    XGetGeometry( QX11Info::display(), child, &root, &x, &y, &w, &h, &border, &depth );
    QPixmap pm( grabWindow( child, x, y, w, h, border, &title, &windowClass ) );
    getWindowsRecursive( windows, child );
    XUngrabServer( QX11Info::display() );

    QPalette p = palette();
    p.setBrush( backgroundRole(), QBrush( pm ) );
    setPalette( p );
    setFixedSize( pm.size() );
    setMouseTracking( true );
    setGeometry( x, y, w, h );
    current = windowIndex( mapFromGlobal(QCursor::pos()) );
}

WindowGrabber::~WindowGrabber()
{
}

QPixmap WindowGrabber::grabCurrent( bool includeDecorations )
{
    Window root;
    int x, y;
    uint w, h, border, depth;
    XGrabServer( QX11Info::display() );
    Window child = windowUnderCursor( includeDecorations );
    XGetGeometry( QX11Info::display(), child, &root, &x, &y, &w, &h, &border, &depth );
    Window parent;
    Window* children;
    unsigned int nchildren;
    if( XQueryTree( QX11Info::display(), child, &root, &parent,
                    &children, &nchildren ) != 0 ) {
	if( children != NULL )
	    XFree( children );
	int newx, newy;
	Window dummy;
	if( XTranslateCoordinates( QX11Info::display(), parent, QX11Info::appRootWindow(),
	    x, y, &newx, &newy, &dummy )) {
	    x = newx;
	    y = newy;
	}
    }
    windowPosition = QPoint(x,y);
    QPixmap pm( grabWindow( child, x, y, w, h, border, &title, &windowClass ) );
    XUngrabServer( QX11Info::display() );
    return pm;
}

void WindowGrabber::mousePressEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::RightButton )
	yPos = e->globalY();
    else {
        if ( current ) {
            windowPosition = e->globalPos() - e->pos() + windows[current].topLeft();
            emit windowGrabbed( palette().brush( backgroundRole() ).texture().copy( windows[ current ] ) );
        } else {
            windowPosition = QPoint(0,0);
            emit windowGrabbed( QPixmap() );
        }
        accept();
    }
}

void WindowGrabber::mouseReleaseEvent( QMouseEvent *e )
{
    if ( e->button() == Qt::RightButton )
	yPos = -1;
}

static
const int minDistance = 10;

void WindowGrabber::mouseMoveEvent( QMouseEvent *e )
{
    if ( yPos == -1 ) {
	int w = windowIndex( e->pos() );
	if ( w != -1 && w != current ) {
	    current = w;
            repaint();
	}
    }
    else {
	int y = e->globalY();
	if ( y > yPos + minDistance ) {
	    decreaseScope( e->pos() );
	    yPos = y;
	}
	else if ( y < yPos - minDistance ) {
	    increaseScope( e->pos() );
	    yPos = y;
	}
    }
}

void WindowGrabber::wheelEvent( QWheelEvent *e )
{
    if ( e->delta() > 0 )
	increaseScope( e->pos() );
    else if ( e->delta() < 0 )
	decreaseScope( e->pos() );
    else
	e->ignore();
}

// Increases the scope to the next-bigger window containing the mouse pointer.
// This method is activated by either rotating the mouse wheel forwards or by
// dragging the mouse forwards while keeping the right mouse button pressed.
void WindowGrabber::increaseScope( const QPoint &pos )
{
    for ( uint i = current + 1; i < windows.size(); i++ ) {
	if ( windows[ i ].contains( pos ) ) {
	    current = i;
	    break;
	}
    }
    repaint();
}

// Decreases the scope to the next-smaller window containing the mouse pointer.
// This method is activated by either rotating the mouse wheel backwards or by
// dragging the mouse backwards while keeping the right mouse button pressed.
void WindowGrabber::decreaseScope( const QPoint &pos )
{
    for ( int i = current - 1; i >= 0; i-- ) {
	if ( windows[ i ].contains( pos ) ) {
	    current = i;
	    break;
	}
    }
    repaint();
}

// Searches and returns the index of the first (=smallest) window
// containing the mouse pointer.
int WindowGrabber::windowIndex( const QPoint &pos ) const
{
    for ( uint i = 0; i < windows.size(); i++ ) {
	if ( windows[ i ].contains( pos ) )
	    return i;
    }
    return -1;
}

// Draws a border around the (child) window currently containing the pointer
void WindowGrabber::paintEvent( QPaintEvent * )
{
    if ( current >= 0 ) {
	QPainter p;
	p.begin( this );
        p.fillRect(rect(), palette().brush( backgroundRole()));
	p.setPen( QPen( Qt::red, 3 ) );
	p.drawRect( windows[ current ].adjusted( 0, 0, -1, -1 ) );
	p.end();
    }
}

#include "windowgrabber.moc"
