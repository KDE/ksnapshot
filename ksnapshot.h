// -*- c++ -*-

#ifndef KSNAPSHOT_H
#define KSNAPSHOT_H
#include "ksnapshotbase.h"
#include "ksnapshotiface.h"
#include <qpixmap.h>
#include <qtimer.h>
#include <dcopclient.h>

class KSnapshot : public KSnapshotBase , virtual public KSnapshotIface
{
  Q_OBJECT

public:
  KSnapshot(QWidget *parent= 0, const char *name= 0);
  ~KSnapshot();

  void slotSave();
  void slotGrab();
  void slotHelp();
  void setTime(int newTime);
  void setURL(QString newURL);
  void setGrabPointer(bool grab);
  void exit();
protected:
    void reject() { close(); }

    virtual void closeEvent( QCloseEvent * e );
    bool eventFilter( QObject*, QEvent* );
    
private slots:
    void grabTimerDone();

private:
    
    void updatePreview();
    void performGrab();

    void autoincFilename();
    QPixmap snapshot;
    QTimer grabTimer;
    QWidget* grabber;
};

#endif // KSNAPSHOT_H

