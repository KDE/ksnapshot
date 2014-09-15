/*
  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
  Copyright (C) 2010, 2011 Pau Garcia i Quiles <pgquiles@elpauer.org>

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

#include <iostream>
#include <algorithm>

#include <QApplication>
#include <QBitmap>
#include <QDebug>
#include <QDesktopWidget>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QScreen>
#include <QWheelEvent>

#ifdef Q_OS_WIN
#include "windows/windowgrabber.cpp"
#elif XCB_XCB_FOUND
#include "xcb/windowgrabber.cpp"
#endif

QString WindowGrabber::title;
QString WindowGrabber::windowClass;
QPoint WindowGrabber::windowPosition;

WindowGrabber::WindowGrabber()
    : QDialog(0, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint),
      current(-1), yPos(-1)
{
    QPixmap pm;
    QRect rect;
    platformSetup(pm, rect);

    QPalette p = palette();
    p.setBrush(backgroundRole(), QBrush(pm));
    setPalette(p);
    setFixedSize(pm.size());
    setMouseTracking(true);
    setGeometry(rect);
    current = windowIndex(mapFromGlobal(QCursor::pos()));
}

WindowGrabber::~WindowGrabber()
{
}

void WindowGrabber::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        yPos = e->globalY();
    } else {
        if (current != -1) {
            windowPosition = e->globalPos() - e->pos() + windows[current].topLeft();
            emit windowGrabbed(palette().brush(backgroundRole()).texture().copy(windows[ current ]));
        } else {
            windowPosition = QPoint(0, 0);
            emit windowGrabbed(QPixmap());
        }
        accept();
    }
}

void WindowGrabber::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        yPos = -1;
    }
}

void WindowGrabber::mouseMoveEvent(QMouseEvent *e)
{
    static const int minDistance = 10;

    if (yPos == -1) {
        int w = windowIndex(e->pos());
        if (w != -1 && w != current) {
            current = w;
            repaint();
        }
    } else {
        int y = e->globalY();
        if (y > yPos + minDistance) {
            decreaseScope(e->pos());
            yPos = y;
        } else if (y < yPos - minDistance) {
            increaseScope(e->pos());
            yPos = y;
        }
    }
}

void WindowGrabber::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0) {
        increaseScope(e->pos());
    } else if (e->delta() < 0) {
        decreaseScope(e->pos());
    } else {
        e->ignore();
    }
}

// Increases the scope to the next-bigger window containing the mouse pointer.
// This method is activated by either rotating the mouse wheel forwards or by
// dragging the mouse forwards while keeping the right mouse button pressed.
void WindowGrabber::increaseScope(const QPoint &pos)
{
    for (uint i = current + 1; i < windows.size(); i++) {
        if (windows[ i ].contains(pos)) {
            current = i;
            break;
        }
    }
    repaint();
}

// Decreases the scope to the next-smaller window containing the mouse pointer.
// This method is activated by either rotating the mouse wheel backwards or by
// dragging the mouse backwards while keeping the right mouse button pressed.
void WindowGrabber::decreaseScope(const QPoint &pos)
{
    for (int i = current - 1; i >= 0; i--) {
        if (windows[ i ].contains(pos)) {
            current = i;
            break;
        }
    }
    repaint();
}

// Searches and returns the index of the first (=smallest) window
// containing the mouse pointer.
int WindowGrabber::windowIndex(const QPoint &pos) const
{
    for (uint i = 0; i < windows.size(); i++) {
        if (windows[ i ].contains(pos)) {
            return i;
        }
    }
    return -1;
}

// Draws a border around the (child) window currently containing the pointer
void WindowGrabber::paintEvent(QPaintEvent *)
{
    if (current >= 0) {
        QPainter p;
        p.begin(this);
        p.fillRect(rect(), palette().brush(backgroundRole()));
        p.setPen(QPen(Qt::red, 3));
        p.drawRect(windows[ current ].adjusted(0, 0, -1, -1));
        p.end();
    }
}

#include "windowgrabber.moc"
