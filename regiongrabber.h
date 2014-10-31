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
    RegionGrabber(const QRect &startSelection);
    ~RegionGrabber();

protected slots:
    void init();

signals:
    void regionGrabbed(const QPixmap &);
    void regionUpdated(const QRect &);

protected:
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    void updateHandles();
    QRegion handleMask(MaskType type) const;
    QPoint limitPointToRect(const QPoint &p, const QRect &r) const;
    QRect normalizeSelection(const QRect &s) const;
    void grabRect();

    QRect selection;
    bool mouseDown;
    bool newSelection;
    const int handleSize;
    QRect *mouseOverHandle;
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

    QVector<QRect *> handles;
    QPixmap pixmap;
};

#endif
