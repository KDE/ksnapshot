/*
 *   Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>,
 *   based on the region grabber code by Luca Gugelmann <lucag@student.ethz.ch>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef FREEREGIONGRABBER_H
#define FREEREGIONGRABBER_H

#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QPolygon>

class QPaintEvent;
class QMouseEvent;

class FreeRegionGrabber : public QWidget
{
    Q_OBJECT

public:
    FreeRegionGrabber(const QPolygon &startFreeRegion);
    ~FreeRegionGrabber();

protected slots:
    void init();

signals:
    void freeRegionGrabbed(const QPixmap &);
    void freeRegionUpdated(const QPolygon &);

protected:
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void grabRect();

    QPolygon selection;
    bool mouseDown;
    bool newSelection;
    const int handleSize;
    QRect *mouseOverHandle;
    QPoint dragStartPoint;
    QPolygon  selectionBeforeDrag;
    bool showHelp;
    bool grabbing;

    QRect helpTextRect;

    QPixmap pixmap;
    QPoint pBefore;
};

#endif
