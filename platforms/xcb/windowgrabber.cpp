/*
  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
  Copyright (C) 2010, 2011 Pau Garcia i Quiles <pgquiles@elpauer.org>
  Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>

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
#include "config-ksnapshot.h"

// std
#include <algorithm>

// xcb
#if XCB_SHAPE_FOUND
#include <xcb/shape.h>
#endif

// Qt
#include <QBitmap>
#include <QPainter>
#include <QX11Info>

// KDE Frameworks
#include <KWindowInfo>

#include "platforms/xcb/pixmaphelper.h"
#include "platforms/xcb/xcbutils.h"

static
bool operator< ( const QRect& r1, const QRect& r2 )
{
    return r1.width() * r1.height() < r2.width() * r2.height();
}

// Recursively iterates over the window w and its children, thereby building
// a tree of window descriptors. Windows in non-viewable state or with height
// or width smaller than minSize will be ignored.
static
void getWindowsRecursive(std::vector<QRect> *windows, xcb_window_t w,
                         int rx = 0, int ry = 0, int depth = 0)
{
    static const int minSize = 8;

    Xcb::WindowAttributes attrs(w);
    Xcb::WindowGeometry geo(w);
    if (!attrs.isNull() && !geo.isNull() &&
        attrs->map_state == XCB_MAP_STATE_VIEWABLE &&
        geo->width >= minSize && geo->height >= minSize) {
        int x = 0, y = 0;
        if (depth) {
            x = geo->x + rx;
            y = geo->y + ry;
        }

        QRect r(x, y, geo->width, geo->height);
        if (std::find(windows->begin(), windows->end(), r) == windows->end()) {
            windows->push_back(r);
        }

        Xcb::Tree tree(w);
        if (!tree.isNull()) {
            xcb_window_t *children = tree.children();
            for (uint16_t i = 0; i < tree->children_len; ++i) {
                getWindowsRecursive(windows, children[i], x, y, depth + 1);
            }
        }
    }

    if (depth == 0) {
        std::sort(windows->begin(), windows->end());
    }
}

static
xcb_atom_t wmStateAtom()
{
    static xcb_atom_t s_atom = XCB_ATOM_NONE;
    if (s_atom == XCB_ATOM_NONE) {
        const QByteArray wmStateName("WM_STATE");
        Xcb::ScopedCPointer<xcb_intern_atom_reply_t> wmState(xcb_intern_atom_reply(
            Xcb::connection(), xcb_intern_atom_unchecked(Xcb::connection(), false, wmStateName.length(), wmStateName.constData()), NULL));
        if (wmState.isNull()) {
            return XCB_ATOM_NONE;
        }
        s_atom = wmState->atom;
    }
    return s_atom;
}

static
xcb_window_t findRealWindow(xcb_window_t w, int depth = 0)
{
    if( depth > 5 ) {
        return XCB_WINDOW_NONE;
    }

    xcb_atom_t wmState = wmStateAtom();
    Xcb::ScopedCPointer<xcb_get_property_reply_t> prop(
        xcb_get_property_reply(
            Xcb::connection(),
            xcb_get_property_unchecked(Xcb::connection(), false, w, wmState, XCB_ATOM_ANY, 0, 0),
            NULL));
    if (!prop.isNull()) {
        if (prop->type != XCB_ATOM_NONE) {
            return w;
        }
    }

    Xcb::Tree tree(w);
    if (tree.isNull()) {
        return XCB_WINDOW_NONE;
    }
    xcb_window_t *children = tree.children();
    for (uint16_t i = 0; i < tree->children_len; ++i) {
        if (xcb_window_t window = findRealWindow(children[i], depth+1)) {
            return window;
        }
    }
    return XCB_WINDOW_NONE;
}

static
xcb_window_t windowUnderCursor(bool includeDecorations = true)
{
    Xcb::ScopedCPointer<xcb_query_pointer_reply_t> pointerReply(
        xcb_query_pointer_reply(Xcb::connection(), xcb_query_pointer_unchecked(Xcb::connection(), QX11Info::appRootWindow()), NULL));

    if (pointerReply.isNull() || pointerReply->child == XCB_WINDOW_NONE) {
        return QX11Info::appRootWindow();
    }

    if (includeDecorations) {
        return pointerReply->child;
    }

    xcb_window_t window = findRealWindow(pointerReply->child);
    return (window != XCB_WINDOW_NONE) ? window : pointerReply->child;
}

static
bool hasShape()
{
#if XCB_SHAPE_FOUND
    xcb_connection_t *c = Xcb::connection();
    xcb_prefetch_extension_data(c, &xcb_shape_id);
    const xcb_query_extension_reply_t *data = xcb_get_extension_data(c, &xcb_shape_id);
    return data->present;
#else
    return false;
#endif
}

static
QPixmap grabWindow(xcb_window_t child, int x, int y, uint w, uint h, uint border,
                   QString *title = 0, QString *windowClass = 0)
{
    QPixmap pm(PixmapHelperXCB::grabWindow(QX11Info::appRootWindow(), x, y, w, h));

    KWindowInfo winInfo(findRealWindow(child), NET::WMVisibleName, NET::WM2WindowClass);

    if (title) {
        (*title) = winInfo.visibleName();
    }

    if (windowClass) {
        (*windowClass) = winInfo.windowClassName();
    }

#if XCB_SHAPE_FOUND
    if (!hasShape()) {
        return pm;
    }
    Xcb::ScopedCPointer<xcb_shape_get_rectangles_reply_t> reply(
        xcb_shape_get_rectangles_reply(Xcb::connection(),
                                       xcb_shape_get_rectangles_unchecked(Xcb::connection(), child, XCB_SHAPE_SK_BOUNDING),
                                       NULL));
    if (reply.isNull()) {
        return pm;
    }
    //The ShapeBounding region is the outermost shape of the window;
    //ShapeBounding - ShapeClipping is defined to be the border.
    //Since the border area is part of the window, we use bounding
    // to limit our work region
    xcb_rectangle_t *rects = xcb_shape_get_rectangles_rectangles(reply.data());

    //Create a QRegion from the rectangles describing the bounding mask.
    QRegion contents;
    for (uint32_t i = 0; i < reply->length; ++i) {
        const xcb_rectangle_t &rect = rects[i];
        contents += QRegion(rect.x, rect.y, rect.width, rect.height);
    }

    //Create the bounding box.
    QRegion bbox(0, 0, w, h);

    if (border > 0) {
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
    QBitmap mask(w, h);
    QPainter p(&mask);
    p.fillRect(0, 0, w, h, Qt::color1);
    for (int pos = 0; pos < maskedAwayRects.count(); pos++) {
        p.fillRect(maskedAwayRects[pos], Qt::color0);
    }
    p.end();

    pm.setMask(mask);
#endif

    return pm;
}

void WindowGrabber::platformSetup(QPixmap &pm, QRect &geom)
{
    int16_t x, y;
    uint16_t w, h;

    uint16_t border;
    x = y = w = h = border = 0;
    xcb_window_t child = XCB_WINDOW_NONE;
    {
        Xcb::ServerGrabber grabber;
        child = windowUnderCursor();
        Xcb::WindowGeometry geo(child);
        if (!geo.isNull()) {
            x = geo->x;
            y = geo->y;
            w = geo->width;
            h = geo->height;
            border = geo->border_width;
        }
    }

    geom = QRect(x, y, w, h);
    pm = grabWindow(child, x, y, w, h, border, &title, &windowClass);
    getWindowsRecursive(&windows, child);
}

QPixmap WindowGrabber::grabCurrent(bool includeDecorations)
{
    Xcb::ServerGrabber grabber;
    xcb_window_t child = windowUnderCursor(includeDecorations);
    if (child == XCB_WINDOW_NONE) {
        return QPixmap();
    }
    Xcb::Tree tree(child);
    Xcb::WindowGeometry geo(child);

    if (tree.isNull() || geo.isNull()) {
        return QPixmap();
    }

    Xcb::ScopedCPointer<xcb_translate_coordinates_reply_t> reply(xcb_translate_coordinates_reply(
        Xcb::connection(),
        xcb_translate_coordinates_unchecked(Xcb::connection(), tree->parent, QX11Info::appRootWindow(), geo->x, geo->y),
        NULL));
    if (reply.isNull()) {
        return QPixmap();
    }

    windowPosition = QPoint(reply->dst_x, reply->dst_y);
    return grabWindow(child, reply->dst_x, reply->dst_y, geo->width, geo->height, geo->border_width, &title, &windowClass);
}
