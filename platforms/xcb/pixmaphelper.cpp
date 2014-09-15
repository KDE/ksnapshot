/*
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

#include "platforms/xcb/pixmaphelper.h"

#include <stdint.h>

#include <xcb/xfixes.h>

#include <QDebug>
#include <QPainter>

#include "platforms/xcb/xcbutils.h"

namespace PixmapHelperXCB
{

void compositePointer(int offsetx, int offsety, QPixmap &snapshot)
{
    Xcb::ScopedCPointer<xcb_xfixes_get_cursor_image_reply_t> cursor(
            xcb_xfixes_get_cursor_image_reply(Xcb::connection(),
                                              xcb_xfixes_get_cursor_image_unchecked(Xcb::connection()),
                                              NULL));

    if (cursor.isNull()) {
        return;
    }

    QImage qcursorimg((uchar *) xcb_xfixes_get_cursor_image_cursor_image(cursor.data()),
                      cursor->width, cursor->height,
                      QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&snapshot);
    painter.drawImage(QPointF(cursor->x - cursor->xhot - offsetx, cursor->y - cursor ->yhot - offsety), qcursorimg);
}

QPixmap grabWindow(WId id, int x, int y, int width, int height)
{
    xcb_connection_t *c = Xcb::connection();
    Xcb::WindowGeometry geo(id);

    if (geo.isNull()) {
        return QPixmap();
    }

    if (width < 0 || width > geo->width) {
        width = geo->width;
    }

    if (width + x > geo->width) {
        width = width + x - geo->width;
    }

    if (height < 0 || height > geo->height) {
        height = geo->height;
    }

    if (height + y > geo->height) {
        height = height + y - geo->height;
    }

    Xcb::ScopedCPointer<xcb_get_image_reply_t> xImage(xcb_get_image_reply(
        c, xcb_get_image_unchecked(c, XCB_IMAGE_FORMAT_Z_PIXMAP, id, x, y, width, height, ~0), NULL));

    if (xImage.isNull()) {
        return QPixmap();
    }

    QImage::Format format = QImage::Format_ARGB32_Premultiplied;
    switch (xImage->depth) {
    case 1:
        format = QImage::Format_MonoLSB;
        break;
    case 30: {
        // Qt doesn't have a matching image format. We need to convert manually
        uint32_t *pixels = reinterpret_cast<uint32_t *>(xcb_get_image_data(xImage.data()));
        for (uint32_t i = 0; i < xImage.data()->length; ++i) {
            int r = (pixels[i] >> 22) & 0xff;
            int g = (pixels[i] >> 12) & 0xff;
            int b = (pixels[i] >>  2) & 0xff;

            pixels[i] = qRgba(r, g, b, 0xff);
        }

        QImage image(reinterpret_cast<uchar *>(pixels), geo->width, geo->height,
                     xcb_get_image_data_length(xImage.data()) / geo->height, QImage::Format_ARGB32_Premultiplied);

        if (image.isNull()) {
            return QPixmap();
        }

        return QPixmap::fromImage(image);
    }
    case 32:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        if (xImage->depth == defaultDepth()) {
            format = findFormat();
            if (format == QImage::Format_Invalid) {
                return QPixmap();
            }
        } else {
            // we don't know
            return QPixmap();
        }
    }

    QImage image(xcb_get_image_data(xImage.data()), width, height,
                 xcb_get_image_data_length(xImage.data()) / height, format);

    if (image.isNull()) {
        return QPixmap();
    }

    if (image.format() == QImage::Format_MonoLSB) {
        // work around an abort in QImage::color
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::white).rgb());
        image.setColor(1, QColor(Qt::black).rgb());
    }

    return QPixmap::fromImage(image);
}

uint8_t defaultDepth()
{
    xcb_connection_t *c = Xcb::connection();
    int screen = QX11Info::appScreen();

    xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(c));
    for (; it.rem; --screen, xcb_screen_next(&it)) {
        if (screen == 0) {
            return it.data->root_depth;
        }
    }
    return 0;
}

QImage::Format findFormat()
{
    xcb_connection_t *c = Xcb::connection();
    int screen = QX11Info::appScreen();

    xcb_screen_iterator_t screenIt = xcb_setup_roots_iterator(xcb_get_setup(c));
    for (; screenIt.rem; --screen, xcb_screen_next(&screenIt)) {
        if (screen != 0) {
            continue;
        }
        xcb_depth_iterator_t depthIt = xcb_screen_allowed_depths_iterator(screenIt.data);
        for (; depthIt.rem; xcb_depth_next(&depthIt)) {
            xcb_visualtype_iterator_t visualIt = xcb_depth_visuals_iterator(depthIt.data);
            for (; visualIt.rem; xcb_visualtype_next(&visualIt)) {
                if (screenIt.data->root_visual != visualIt.data->visual_id) {
                    continue;
                }
                xcb_visualtype_t *visual = visualIt.data;
                if ((depthIt.data->depth == 24 || depthIt.data->depth == 32) &&
                        visual->red_mask   == 0x00ff0000 &&
                        visual->green_mask == 0x0000ff00 &&
                        visual->blue_mask  == 0x000000ff) {
                    return QImage::Format_ARGB32_Premultiplied;
                }
                if (depthIt.data->depth == 16 &&
                        visual->red_mask   == 0xf800 &&
                        visual->green_mask == 0x07e0 &&
                        visual->blue_mask  == 0x001f) {
                    return QImage::Format_RGB16;
                }
                break;
            }
        }
    }
    return QImage::Format_Invalid;
}

} // namespace PixmapHelperXCB

