/*
 *  Copyright (C) 2007 Montel Laurent <montel@kde.org>
 *  Copyright (C) 2010 Pau Garcia i Quiles <pgquiles@elpauer.org>
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

#include "kbackgroundsnapshot.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QPixmap>
#include <QScreen>
#include <QStandardPaths>

#include <KLocalizedString>
#include <KAboutData>


#include "regiongrabber.h"
#include "ksnapshot_options.h"
#include "windowgrabber.h"

KBackgroundSnapshot::KBackgroundSnapshot(KSnapshotObject::CaptureMode mode)
    : KSnapshotObject(),
      m_captureMode(mode)
{
    m_grabber = new QWidget(0, Qt::X11BypassWindowManagerHint);
    m_grabber->move(-1000, -1000);
    m_grabber->installEventFilter(this);
    m_grabber->show();
    m_grabber->grabMouse(Qt::WaitCursor);

    if (mode == KSnapshotObject::FullScreen) {
        grabFullScreen();
        savePictureOnDesktop();
    } else {
        switch (mode) {
            case KSnapshotObject::WindowUnderCursor:
                performGrab();
                break;
            case  KSnapshotObject::ChildWindow:
                slotGrab();
                break;
            case KSnapshotObject::Region:
                grabRegion();
                break;
            default:
                break;
        }
    }

    //When we use argument to take m_snapshot we mustn't hide it.
    if (mode != KSnapshotObject::ChildWindow) {
        m_grabber->releaseMouse();
        m_grabber->hide();
    }

}

KBackgroundSnapshot::~KBackgroundSnapshot()
{
    ////qDebug()<<" KBackgroundSnapshot::~KBackgroundSnapshot()\n";
}

void KBackgroundSnapshot::savePictureOnDesktop()
{
    m_filename = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + '/' + i18n("snapshot") + "1.png");
    // Make sure the name is not already being used
    autoincFilenameUntilUnique(0);
    save(m_filename, 0L);
    exit(0);
}

void KBackgroundSnapshot::grabFullScreen()
{
    const QList<QScreen *> screens = qApp->screens();
    const QDesktopWidget *desktop = QApplication::desktop();
    const int screenId = desktop->screenNumber(QCursor::pos());
    if (screenId < screens.count()) {
        m_snapshot = screens[screenId]->grabWindow(desktop->winId());
    }
}

void KBackgroundSnapshot::performGrab()
{
    ////qDebug()<<"KBackgroundSnapshot::performGrab()\n";
    m_grabber->releaseMouse();
    m_grabber->hide();
    if (m_captureMode == ChildWindow) {
        WindowGrabber wndGrab;
        connect(&wndGrab, &WindowGrabber::windowGrabbed, this, &KBackgroundSnapshot::slotWindowGrabbed);
        wndGrab.exec();
        savePictureOnDesktop();
    } else if (m_captureMode == WindowUnderCursor) {
        m_snapshot = WindowGrabber::grabCurrent(true);
        savePictureOnDesktop();
    } else {
        grabFullScreen();
        savePictureOnDesktop();
    }
}

void KBackgroundSnapshot::slotWindowGrabbed(const QPixmap &pix)
{
    ////qDebug()<<" KBackgroundSnapshot::slotWindowGrabbed( const QPixmap &pix )\n";
    if (!pix.isNull()) {
        m_snapshot = pix;
    }
    QApplication::restoreOverrideCursor();
}


void KBackgroundSnapshot::slotGrab()
{
    ////qDebug()<<"KBackgroundSnapshot::slotGrab()\n";
    m_grabber->show();
    m_grabber->grabMouse(Qt::CrossCursor);
}


void KBackgroundSnapshot::grabRegion()
{
    QRect emptySelection;
    m_regionGrab = new RegionGrabber(emptySelection);
    connect(m_regionGrab, &RegionGrabber::regionGrabbed, this, &KBackgroundSnapshot::slotRegionGrabbed);

}


void KBackgroundSnapshot::slotRegionGrabbed(const QPixmap &pix)
{
    if (!pix.isNull()) {
        m_snapshot = pix;
    }

    m_regionGrab->deleteLater();
    QApplication::restoreOverrideCursor();
    savePictureOnDesktop();
}

bool KBackgroundSnapshot::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_grabber && event->type() == QEvent::MouseButtonPress) {
        if (QWidget::mouseGrabber() != m_grabber) {
            return false;
        }

        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            performGrab();
        }
    }

    return false;
}


#define KBACKGROUNDSNAPVERSION "0.1"

static const char description[] = I18N_NOOP("KDE Background Screenshot Utility");

int main(int argc, char **argv)
{
    KAboutData aboutData("kbackgroundsnapshot", i18n("KBackgroundSnapshot"),
                         KBACKGROUNDSNAPVERSION, i18n(description), KAboutLicense::GPL,
                         i18n("(c) 2007, Montel Laurent"));

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    addCommandLineOptions(parser);    // Add our own options.

    if (parser.isSet("current")) {
        new KBackgroundSnapshot(KSnapshotObject::WindowUnderCursor);
    } else if (parser.isSet("fullscreen")) {
        new KBackgroundSnapshot(KSnapshotObject::FullScreen);
    } else if (parser.isSet("region")) {
        new KBackgroundSnapshot(KSnapshotObject::Region);
    } else if (parser.isSet("freeregion")) {
        new KBackgroundSnapshot(KSnapshotObject::FreeRegion);
    } else if (parser.isSet("child")) {
        new KBackgroundSnapshot(KSnapshotObject::ChildWindow);
    } else {
        new KBackgroundSnapshot();
    }

    return app.exec();
}
