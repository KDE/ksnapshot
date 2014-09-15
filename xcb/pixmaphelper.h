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
#ifndef PIXMAPHELPER_XCB_H
#define PIXMAPHELPER_XCB_H

#include <QPixmap>
#include <QImage>

namespace PixmapHelperXCB
{
    void compositePointer(int offsetX, int offsetY, QPixmap &snapshot);
    QPixmap grabWindow(WId window, int x = 0, int y = 0, int width = -1, int height = -1);

    uint8_t defaultDepth();
    QImage::Format findFormat();
};

#endif
