// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H

#include <qlabel.h>
#include <qpixmap.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QCloseEvent>

#include <kglobalsettings.h>
#include <kdialog.h>
#include <kurl.h>

class RegionGrabber;
class KSnapshotWidget;

class KSnapshotPreview : public QLabel
{
    Q_OBJECT

    public:
        KSnapshotPreview(QWidget *parent)
            : QLabel(parent)
        {
            setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        }
        virtual ~KSnapshotPreview() {}

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

public:
	int grabMode();
	int timeout();
private:
    bool save( const KUrl& url );
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

