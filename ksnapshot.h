// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H
#include "ksnapshotbase.h"
#include "ksnapshotiface.h"
#include <qlabel.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <dcopclient.h>
#include <kglobalsettings.h>

class KSnapshotThumb : public QLabel
{
    Q_OBJECT

    public:
        KSnapshotThumb(QWidget *parent, const char *name = 0)
            : QLabel(parent, name)
        {}
        virtual ~KSnapshotThumb() {}
    
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

class KSnapshot : public KSnapshotBase , virtual public KSnapshotIface
{
  Q_OBJECT

public:
  KSnapshot(QWidget *parent= 0, const char *name= 0);
  ~KSnapshot();

  bool save( const QString &filename );

//public slots:
  void slotSave();
  void slotSaveAs();
  void slotGrab();
  void slotHelp();
  void slotCopy();
  void slotPrint();
  void setTime(int newTime);
  void setURL(QString newURL);
  void setGrabPointer(bool grab);
  void slotMovePointer( int x, int y );
  void exit();

protected:
    void reject() { close(); }

    virtual void closeEvent( QCloseEvent * e );
    bool eventFilter( QObject*, QEvent* );
    
private slots:
    void grabTimerDone();
    void slotDragSnapshot();

private:
    
    void updatePreview();
    void performGrab();

    void autoincFilename();
    QPixmap snapshot;
    QTimer grabTimer;
    QWidget* grabber;
    QString filename;
};

#endif // KSNAPSHOT_H

