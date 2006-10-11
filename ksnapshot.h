/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H

#include <QBitmap>
#include <QLabel>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>
#include <QMouseEvent>
#include <QPixmap>

#include <kglobalsettings.h>
#include <kdialog.h>
#include <kurl.h>

class RegionGrabber;
class KSnapshotWidget;

#include "kdebug.h"
class KSnapshotPreview : public QLabel
{
    Q_OBJECT

    public:
        KSnapshotPreview(QWidget *parent)
            : QLabel(parent)
        {
            setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            setCursor(Qt::OpenHandCursor);
        }
        virtual ~KSnapshotPreview() {}

        void setPreview(QPixmap pixmap)
        {
            // if this looks convoluted, that's because it is. drawing a PE_SizeGrip
            // does unexpected things when painting directly onto the pixmap
            QPixmap handle(15, 15);
            QBitmap mask(15, 15);
            mask.clear();
            QStyleOption o;
            o.rect = QRect(0, 0, 15, 15);

            {
                QPainter p(&mask);
                style()->drawControl(QStyle::CE_SizeGrip, &o, &p);
                p.end();
                handle.setMask(mask);
            }

            {
                QPainter p(&handle);
                style()->drawControl(QStyle::CE_SizeGrip, &o, &p);
                p.end();
            }

            o.rect = QRect(pixmap.width() - 16, pixmap.height() - 16, 15, 15);
            QPainter p(&pixmap);
            p.drawPixmap(o.rect, handle);
            p.end();

            // hooray for making things like setPixmap not virtual! *sigh*
            setPixmap(pixmap);
        }

    signals:
        void startDrag();

    protected:
        void mousePressEvent(QMouseEvent * e)
        {
            if ( e->button() == Qt::LeftButton )
                mClickPt = e->pos();
        }

        void mouseMoveEvent(QMouseEvent * e)
        {
            if (mClickPt != QPoint(0, 0) &&
                (e->pos() - mClickPt).manhattanLength() > KGlobalSettings::dndEventDelay())
            {
                mClickPt = QPoint(0, 0);
                emit startDrag();
            }
        }

        void mouseReleaseEvent(QMouseEvent * /*e*/)
        {
            mClickPt = QPoint(0, 0);
        }

        QPoint mClickPt;
};

class KSnapshot : public KDialog
{
  Q_OBJECT

public:
    KSnapshot(QWidget *parent= 0, bool grabCurrent=false);
    ~KSnapshot();

    enum CaptureMode { FullScreen=0, WindowUnderCursor=1, Region=2, ChildWindow=3 };

    bool save( const QString &filename );
    QString url() const { return filename.url(); }

public slots:
    void slotGrab();
    void slotSave();
    void slotSaveAs();
    void slotCopy();
    void slotOpen();
    void slotMovePointer( int x, int y );
    void setTime( int newTime );
    void setURL( const QString &newURL );
    void setGrabMode( int m );
    void exit();

protected:
    void reject() { close(); }
    virtual void closeEvent( QCloseEvent * e );
    void resizeEvent(QResizeEvent*);
    bool eventFilter( QObject*, QEvent* );

private slots:
    void grabTimerDone();
    void slotDragSnapshot();
    void updateCaption();
    void updatePreview();
    void slotRegionGrabbed( const QPixmap & );
    void slotWindowGrabbed( const QPixmap & );
    void slotModeChanged( int mode );
    void setPreview( const QPixmap &pm );
    void setDelay( int i );
    void setIncludeDecorations( bool b );
    void setMode( int mode );
    int delay();
    bool includeDecorations();
    int mode();
    QPixmap preview();
    int previewWidth();
    int previewHeight();

public:
    int grabMode();
    int timeout();

private:
    bool save( const KUrl& url );
    bool saveEqual( const KUrl& url );
    void performGrab();
    void autoincFilename();
    QPixmap snapshot;
    QTimer grabTimer;
    QTimer updateTimer;
    QWidget* grabber;
    KUrl filename;
    KSnapshotWidget *mainWidget;
    RegionGrabber *rgnGrab;
    bool modified;
};

#endif // KSNAPSHOT_H
