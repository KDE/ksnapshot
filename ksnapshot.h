/*
 *  Copyright (C) 1997-2002 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ kde.org>
 *
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

#include <QAction>
#include <QDialog>
#include <QPixmap>
#include <KService>
#include <QTimer>
#include <QUrl>

#include "ksnapshotobject.h"
#include "snapshottimer.h"

#include "config-ksnapshot.h"

class KSnapshotWidget;
class QMenu;

#ifdef KIPI_FOUND
#include "ksnapshotimagecollectionshared.h"
#include <libkipi/pluginloader.h>
#endif

class KSnapshotServiceAction : public QAction
{
    Q_OBJECT
public:
    KSnapshotServiceAction(KService::Ptr s, QObject *parent)
        : QAction(parent), service(s) {}
    KSnapshotServiceAction(KService::Ptr s,
                           const QString &text,
                           QObject *parent)
        : QAction(text, parent), service(s) {}
    KSnapshotServiceAction(KService::Ptr s,
                           const QIcon &icon,
                           const QString &text,
                           QObject *parent)
        : QAction(icon, text, parent), service(s) {}

    KService::Ptr service;
};

class KSnapshot : public QDialog, public KSnapshotObject
{
    Q_OBJECT

public:
    explicit KSnapshot(QWidget *parent = 0, KSnapshotObject::CaptureMode mode = FullScreen);
    ~KSnapshot();


    QString url() const
    {
        return m_filename.url();
    }

public slots:
    void slotGrab();
    void slotSave();
    void slotSaveAs();
    void slotCopy();
    void slotOpen(const QString &application);
    void slotMovePointer(int x, int y);
    void setTime(int newTime);
    void setURL(const QString &newURL);
    void setGrabMode(int m);
    void exit();

protected:
    void reject()
    {
        close();
    }
    virtual void closeEvent(QCloseEvent *e);
    void resizeEvent(QResizeEvent *);
    bool eventFilter(QObject *, QEvent *);

protected Q_SLOTS:
    virtual void refreshCaption();

private slots:
    void delayedInit();
    void slotOpen(QAction *);
    void slotPopulateOpenMenu();
    void m_grabTimerDone();
    void slotDragSnapshot();
    void updatePreview();
    void slotRegionGrabbed(const QPixmap &);
    void slotRegionUpdated(const QRect &);
    void slotFreeRegionUpdated(const QPolygon &);
    void slotWindowGrabbed(const QPixmap &);
    void slotModeChanged(int mode);
    void setPreview(const QPixmap &pm);
    void setDelay(int i);
    void setIncludeDecorations(bool b);
    void setIncludePointer(bool b);
    bool includePointer() const;
    void setMode(int mode);
    int delay() const;
    bool includeDecorations() const;
    int mode() const;
    QPixmap preview();
    int previewWidth() const;
    int previewHeight() const;
    void startUndelayedGrab();
    void slotScreenshotReceived(qulonglong handle);
    void setDelaySpinboxSuffix(int value);

public:
    int grabMode() const;
    int timeout() const;

private:
    QUrl urlToOpen(bool *isTempfile = 0);
    void performGrab();
    void grabPointerImage(int offsetx, int offsety);
    void grabFullScreen();
    void grabCurrentScreen();
    void grabRegion();
    void grabFreeRegion();

    SnapshotTimer m_grabTimer;
    QTimer m_updateTimer;
    QMenu  *m_openMenu;
    KSnapshotWidget *m_snapshotWidget;
    bool m_modified;
    QPoint m_savedPosition;
    bool m_haveXFixes;
    bool m_includeAlpha;
    QPolygon m_lastFreeRegion;
    QRect m_lastRegion;

#ifdef KIPI_FOUND
    KIPI::PluginLoader *m_pluginLoader;
    friend QList<QUrl> KSnapshotImageCollectionShared::images();
#endif
};

#endif // KSNAPSHOT_H
