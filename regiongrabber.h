/*
 *   Copyright (C) 2007 Luca Gugelmann <lucag@student.ethz.ch>
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

#ifndef REGIONGRABBER_H
#define REGIONGRABBER_H

#include <QWidget>
#include <QRegion>
#include <QPoint>
#include <QVector>
#include <QRect>

class QPaintEvent;
class QResizeEvent;
class QMouseEvent;

class RegionGrabber : public QWidget
{
    Q_OBJECT

    enum MaskType { StrokeMask, FillMask };
    
public:
    RegionGrabber();
    ~RegionGrabber();

protected slots:
    void init();

signals:
    void regionGrabbed( const QPixmap & );

protected:
    void paintEvent( QPaintEvent* e );
    void resizeEvent( QResizeEvent* e );
    void mousePressEvent( QMouseEvent* e );
    void mouseMoveEvent( QMouseEvent* e );
    void mouseReleaseEvent( QMouseEvent* e );
    void mouseDoubleClickEvent( QMouseEvent* );
    void keyPressEvent( QKeyEvent* e );
    void updateHandles();
    QRegion handleMask( MaskType type ) const;
    QPoint limitPointToRect( const QPoint &p, const QRect &r ) const;
    QRect normalizeSelection( const QRect &s ) const;
    void grabRect();

    QRect selection;
    bool mouseDown;
    bool newSelection;
    const int handleSize;
    QRect* mouseOverHandle;
    QPoint dragStartPoint;
    QRect  selectionBeforeDrag;
    bool showHelp;
    bool grabbing;

    // naming convention for handles
    // T top, B bottom, R Right, L left
    // 2 letters: a corner
    // 1 letter: the handle on the middle of the corresponding side
    QRect TLHandle, TRHandle, BLHandle, BRHandle;
    QRect LHandle, THandle, RHandle, BHandle;
    QRect helpTextRect;

    QVector<QRect*> handles;
    QPixmap pixmap;
};

#endif
