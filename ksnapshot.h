// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H
#include "ksnapshotiface.h"

#include <qbitmap.h>
#include <qcursor.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>
#include <qtimer.h>

#include <dcopclient.h>
#include <kglobalsettings.h>
#include <kdialogbase.h>
#include <kurl.h>

class RegionGrabber;
class KSnapshotWidget;

class KSnapshotPreview : public QLabel
{
    Q_OBJECT

    public:
        KSnapshotPreview(QWidget *parent, const char *name = 0)
            : QLabel(parent, name)
        {
            setAlignment(AlignHCenter | AlignVCenter);
            setCursor(QCursor(Qt::PointingHandCursor));
        }
        virtual ~KSnapshotPreview() {}

        void setPixmap(const QPixmap& pm)
        {
            // if this looks convoluted, that's because it is. drawing a PE_SizeGrip
            // does unexpected things when painting directly onto the pixmap
            QPixmap pixmap(pm);
            QPixmap handle(15, 15);
            QBitmap mask(15, 15, true);

            {
                QPainter p(&mask);
                style().drawPrimitive(QStyle::PE_SizeGrip, &p, QRect(0, 0, 15, 15), palette().active());
                p.end();
                handle.setMask(mask);
            }

            {
                QPainter p(&handle);
                style().drawPrimitive(QStyle::PE_SizeGrip, &p, QRect(0, 0, 15, 15), palette().active());
                p.end();
            }

            QRect rect(pixmap.width() - 16, pixmap.height() - 16, 15, 15);
            QPainter p(&pixmap);
            p.drawPixmap(rect, handle);
            p.end();
            QLabel::setPixmap(pixmap);
        }

    signals:
        void startDrag();

    protected:
        void mousePressEvent(QMouseEvent * e)
        {
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

class KSnapshot : public KDialogBase, virtual public KSnapshotIface
{
  Q_OBJECT

public:
  KSnapshot(QWidget *parent= 0, const char *name= 0, bool grabCurrent=false);
  ~KSnapshot();

  enum CaptureMode { FullScreen=0, WindowUnderCursor=1, Region=2, ChildWindow=3 };

  bool save( const QString &filename );
  QString url() const { return filename.url(); }

protected slots:
  void slotGrab();
  void slotSave();
  void slotSaveAs();
  void slotCopy();
  void slotPrint();
  void slotMovePointer( int x, int y );

  void setTime(int newTime);
  void setURL(const QString &newURL);
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

private:
    bool save( const KURL& url );
    void performGrab();
    void autoincFilename();
    int grabMode();
    int timeout();

    QPixmap snapshot;
    QTimer grabTimer;
    QTimer updateTimer;
    QWidget* grabber;
    KURL filename;
    KSnapshotWidget *mainWidget;
    RegionGrabber *rgnGrab;
    bool modified;
};

#endif // KSNAPSHOT_H

