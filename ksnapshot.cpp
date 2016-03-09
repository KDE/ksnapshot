/*
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2010 Martin Gräßlin <kde@martin-graesslin.com>
 *  Copyright (C) 2010, 2011 Pau Garcia i Quiles <pgquiles@elpauer.org>
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

#include "ksnapshot.h"

#include "config-ksnapshot.h"

#include <QClipboard>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QShortcut>
#include <QMenu>
#include <QDesktopWidget>
#include <QVarLengthArray>
#include <QCloseEvent>
#include <QDrag>
#include <QFileDialog>
#include <QImageWriter>
#include <QMimeDatabase>
#include <QMimeData>
#include <QMimeType>
#include <QMouseEvent>
#include <QPainter>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QtCore/QXmlStreamReader>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QScreen>
#include <QWindow>

#include <KAboutData>
#include <KConfigGroup>
#include <KGuiItem>
#include <KHelpMenu>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KOpenWithDialog>
#include <KRun>
#include <KSharedConfig>
#include <KStandardGuiItem>
#include <KStandardShortcut>
#include <KStartupInfo>
#include <KWindowConfig>

#include "regiongrabber.h"
#include "freeregiongrabber.h"
#include "windowgrabber.h"
#include "ksnapshotpreview.h"
#include "ksnapshotsendtoactions.h"
#include "ui_ksnapshotwidget.h"

#if HAVE_X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <QX11Info>
#endif

#if XCB_XCB_FOUND
#include <platforms/xcb/pixmaphelper.h>
#endif

class KSnapshotWidget : public QWidget, public Ui::KSnapshotWidget
{
public:
    KSnapshotWidget(QWidget *parent = 0)
        : QWidget(parent)
    {
        setupUi(this);
        btnNew->setIcon(QIcon::fromTheme("ksnapshot"));
    }
};

KSnapshot::KSnapshot(KSnapshotObject::CaptureMode mode, QWidget *parent)
    : QDialog(parent),
      KSnapshotObject(),
      m_modified(true),
      m_savedPosition(QPoint(-1, -1))
{
    setWindowTitle(i18nc("untitled snapshot", "untitled"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Apply);
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    QPushButton *copyToClipboardButton = new QPushButton;
    buttonBox->addButton(copyToClipboardButton, QDialogButtonBox::ActionRole);
    QPushButton *sendToButton = new QPushButton;
    buttonBox->addButton(sendToButton, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    KGuiItem::assign(buttonBox->button(QDialogButtonBox::Apply), KStandardGuiItem::saveAs());
    KGuiItem::assign(copyToClipboardButton, KGuiItem(i18n("Copy"), "edit-copy"));
    KGuiItem::assign(sendToButton, KGuiItem(i18n("Send To..."), "document-open"));
    buttonBox->button(QDialogButtonBox::Apply)->setDefault(true);
    buttonBox->button(QDialogButtonBox::Apply)->setShortcut(Qt::CTRL | Qt::Key_Return);
    m_grabber = new QWidget(0, Qt::X11BypassWindowManagerHint);

    // TODO X11 (Xinerama and Twinview, actually) and Windows use different coordinates for the two monitors case
    //
    // On Windows, there are two displays. The origin (0, 0) ('o') is the top left of display 1. If display 2 is to the left, then coordinates in display 2 are negative:
    //  .-------.
    //  |       |o-----.
    //  |   2   |      |
    //  |       |   1  |
    //  ._______.._____.
    //
    // On Xinerama and Twinview, there is only one display and two screens. The origin (0, 0) ('o') is the top left of the display:
    //  o-------.
    //  |       |.-----.
    //  |   2   |      |
    //  |       |   1  |
    //  ._______.._____.
    //
    // Instead of moving to (-10000, -10000), we should compute how many displays are and make sure we move to somewhere out of the total coordinates.
    //   - Windows: use GetSystemMetrics ( http://msdn.microsoft.com/en-us/library/ms724385(v=vs.85).aspx )

    // If moving to a negative position, we need to count the size of the dialog; moving to a positive position avoids having to compute the size of the dialog

    m_grabber->move(-10000, -10000);    // FIXME Read above

    m_grabber->installEventFilter(this);

    KStartupInfo::appStarted();

    m_snapshotWidget = new KSnapshotWidget(this);

    connect(m_snapshotWidget->lblImage, &KSnapshotPreview::startDrag, this, &KSnapshot::slotDragSnapshot);
    connect(m_snapshotWidget->btnNew, &QPushButton::clicked, this, &KSnapshot::slotGrab);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &KSnapshot::slotSaveAs);
    connect(copyToClipboardButton, &QPushButton::clicked, this, &KSnapshot::slotCopy);
    connect(m_snapshotWidget->comboMode, (void (QComboBox:: *)(int)) &QComboBox::activated, this, &KSnapshot::slotModeChanged);

    if (qApp->desktop()->numScreens() < 2) {
        m_snapshotWidget->comboMode->removeItem(CurrentScreen);
    }

    m_openMenu = new QMenu(this);
    sendToButton->setMenu(m_openMenu);
    connect(m_openMenu, &QMenu::aboutToShow, this, &KSnapshot::slotPopulateOpenMenu);
    connect(m_openMenu, &QMenu::triggered, this, (void (KSnapshot:: *)(QAction *)) &KSnapshot::slotOpen);

    connect(m_snapshotWidget->spinDelay, (void (QSpinBox:: *)(int)) &QSpinBox::valueChanged, this, &KSnapshot::setDelaySpinboxSuffix);
    setDelaySpinboxSuffix(m_snapshotWidget->spinDelay->value());

    mainLayout->addWidget(m_snapshotWidget);
    mainLayout->addWidget(buttonBox);

    m_grabber->show();
    m_grabber->grabMouse();

    KConfigGroup conf(KSharedConfig::openConfig(), "GENERAL");

#if HAVE_X11
    {
        // prevent KWin from animating the window in the compositor
        Display *dpy = QX11Info::display();
        Atom atom = XInternAtom(dpy, "_KDE_NET_WM_SKIP_CLOSE_ANIMATION", False);
        long d = 1;
        XChangeProperty(dpy, winId(), atom, XA_CARDINAL, 32,
                        PropModeReplace, (unsigned char *) &d, 1);
    }
#elif !defined(Q_OS_WIN)
    m_snapshotWidget->cbIncludePointer->hide();
    m_snapshotWidget->lblIncludePointer->hide();
#endif
    setIncludePointer(conf.readEntry("includePointer", false));

    // check if kwin screenshot effect is available
    m_useKwinEffect = false;
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.KWin")) {
        QDBusInterface kwinInterface("org.kde.KWin", "/", "org.freedesktop.DBus.Introspectable");
        QDBusReply<QString> reply = kwinInterface.call("Introspect");
        if (reply.isValid()) {
            QXmlStreamReader xml(reply.value());
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.tokenType() == QXmlStreamReader::StartElement &&
                        xml.name().toString() == "node") {
                    if (xml.attributes().value("name").toString() == "Screenshot") {
                        m_useKwinEffect = true;
                        break;
                    }
                }
            }
        }
    }

    //qDebug() << "Mode = " << mode;
    setMode(conf.readEntry("mode", 0));
    if (mode == KSnapshotObject::FullScreen) {
        grabFullScreen();
    } else if (mode == KSnapshotObject::CurrentScreen) {
        grabCurrentScreen();
    } else {
        setMode(mode);
        switch (mode) {
        case KSnapshotObject::WindowUnderCursor: {
            setIncludeDecorations(true);
            performGrab();
            break;
        }
        case  KSnapshotObject::ChildWindow: {
            slotGrab();
            break;
        }
        case KSnapshotObject::Region: {
            grabRegion();
            break;
        }
        case KSnapshotObject::FreeRegion: {
            grabFreeRegion();
            break;
        }
        default:
            break;
        }
    }

    //When we use argument to take m_snapshot we mustn't hide it.
    if (mode !=  KSnapshotObject::ChildWindow) {
        m_grabber->releaseMouse();
        m_grabber->hide();
    }

    setDelay(conf.readEntry("delay", 0));
    setIncludeDecorations(conf.readEntry("includeDecorations", true));
    m_filename = QUrl::fromLocalFile(conf.readPathEntry("m_filename", QDir::currentPath() + '/' + i18n("snapshot") + "1.png"));

    connect(&m_grabTimer, &SnapshotTimer::timeout, this, &KSnapshot::grabTimerDone);
    connect(&m_updateTimer, &QTimer::timeout, this, &KSnapshot::updatePreview);

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    buttonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    QList<QKeySequence> shortcuts = KStandardShortcut::shortcut(KStandardShortcut::Quit);
    new QShortcut(shortcuts.empty() ? QKeySequence() : shortcuts.first(), this, SLOT(reject()));

    new QShortcut(Qt::Key_Q, this, SLOT(slotSave()));

    shortcuts = KStandardShortcut::shortcut(KStandardShortcut::Copy);
    new QShortcut(shortcuts.empty() ? QKeySequence() : shortcuts.first(), copyToClipboardButton, SLOT(animateClick()));

    shortcuts = KStandardShortcut::shortcut(KStandardShortcut::Save);
    new QShortcut(shortcuts.empty() ? QKeySequence() : shortcuts.first(), buttonBox->button(QDialogButtonBox::Apply), SLOT(animateClick()));
    new QShortcut(Qt::Key_S, buttonBox->button(QDialogButtonBox::Apply), SLOT(animateClick()));

    shortcuts = KStandardShortcut::shortcut(KStandardShortcut::New);
    new QShortcut(shortcuts.empty() ? QKeySequence() : shortcuts.first(), m_snapshotWidget->btnNew, SLOT(animateClick()));

    new QShortcut(Qt::Key_N, m_snapshotWidget->btnNew, SLOT(animateClick()));
    new QShortcut(Qt::Key_Space, m_snapshotWidget->btnNew, SLOT(animateClick()));

    m_snapshotWidget->btnNew->setFocus();
    resize(QSize(250, 500));

    m_sendToActions = QSharedPointer<KSnapshotSendToActions>::create();
#ifdef KIPI_FOUND
    m_sendToActions->setKSnapshotForKipi(this); // todo: better remove this dependency
#endif

    QMetaObject::invokeMethod(this, "delayedInit", Qt::QueuedConnection);
}

KSnapshot::~KSnapshot()
{
    delete m_snapshotWidget;
}

QString KSnapshot::url() const
{
    return m_filename.url();
}

void KSnapshot::delayedInit()
{
    // calling restoreWindowSize in the ctor doesn't work
    // doing it in a delayed slot does.
    KConfigGroup cg(KSharedConfig::openConfig(), "MainWindow");
    KWindowConfig::restoreWindowSize(windowHandle(), cg);
    refreshCaption();
}

void KSnapshot::setDelaySpinboxSuffix(int value)
{
    m_snapshotWidget->spinDelay->setSuffix(i18np(" second", " seconds", value));
}

void KSnapshot::resizeEvent(QResizeEvent *)
{
    m_updateTimer.setSingleShot(true);
    m_updateTimer.start(200);
}

void KSnapshot::slotSave()
{
    // Make sure the name is not already being used
    autoincFilenameUntilUnique(this);
    if (save(m_filename, this)) {
        m_modified = false;
    }
}

void KSnapshot::slotSaveAs()
{
    //TODO: non-blocking save
    QStringList filters;
    QMimeDatabase db;
    QString selectedFilter;
    const QString mimeTypeForFilename = db.mimeTypeForUrl(m_filename).name();
    for (auto mimetype: QImageWriter::supportedMimeTypes()) {
        filters << db.mimeTypeForName(mimetype).filterString();
        if (mimetype == mimeTypeForFilename) {
            selectedFilter = filters.last(); // pre-select the right MIME type (image type format) in the dialog
        }
    }

    const QUrl url = QFileDialog::getSaveFileUrl(this, i18n("Save Snapshot As"), m_filename, filters.join(";;"), &selectedFilter);

    if (!url.isValid()) {
        return;
    }

    if (save(url, this)) {
        m_modified = false;
    }
}

void KSnapshot::slotCopy()
{
    QApplication::clipboard()->setPixmap(m_snapshot);
}

void KSnapshot::slotDragSnapshot()
{
    QDrag *drag = new QDrag(this);

    drag->setMimeData(new QMimeData);
    drag->mimeData()->setImageData(m_snapshot);
    drag->mimeData()->setData("application/x-kde-suggestedfilename", QFile::encodeName(m_filename.fileName()));
    drag->setPixmap(preview());
    QList<QUrl> urls;
    urls << urlToOpen();
    drag->mimeData()->setUrls(urls);
    drag->start();
}

void KSnapshot::slotGrab()
{
    m_savedPosition = pos();
    hide();

    if (delay()) {
        //qDebug() << "starting timer with time of" << delay();
        m_grabTimer.start(delay());
    } else {
        QMetaObject::invokeMethod(this, "startUndelayedGrab", Qt::QueuedConnection);
    }
}

void KSnapshot::startUndelayedGrab()
{
    if (mode() == Region) {
        grabRegion();
    } else if (mode() == FreeRegion) {
        grabFreeRegion();
    } else {
        m_grabber->show();
        m_grabber->grabMouse(Qt::CrossCursor);
    }
}

QUrl KSnapshot::urlToOpen(bool *isTempfile)
{
    if (isTempfile) {
        *isTempfile = false;
    }

    if (!m_modified && urlExists(m_successfulSaveUrl, this)) {
        return m_successfulSaveUrl;
    }

    QTemporaryFile tmpFile(QDir::tempPath()+"/snapshot_XXXXXX.png");
    tmpFile.setAutoRemove(false); // Do not remove file when the tmpFile object gets destroyed. KRun in the calling code will do that.
    tmpFile.open();
    const QUrl path = QUrl::fromLocalFile(tmpFile.fileName());

    if (saveTo(path, this)) {
        if (isTempfile) {
            *isTempfile = true;
        }

        return path;
    }

    return QUrl();
}

void KSnapshot::slotOpen(const QString &application)
{
    QUrl url = urlToOpen();
    if (!url.isValid()) {
        return;
    }

    QList<QUrl> list;
    list.append(url);
    KRun::run(application, list, this);
}

void KSnapshot::slotOpen(QAction *action)
{
    KSnapshotServiceAction *serviceAction = qobject_cast<KSnapshotServiceAction *> (action);

    if (!serviceAction) {
        return;
    }

    bool isTempfile = false;
    QUrl url = urlToOpen(&isTempfile);
    if (!url.isValid()) {
        return;
    }

    QList<QUrl> list;
    list.append(url);

    KService::Ptr service = serviceAction->service;
    if (!service) {
        QSharedPointer<KOpenWithDialog> dlg(new KOpenWithDialog(list, this));
        if (!dlg->exec()) {
            return;
        }

        service = dlg->service();

        if (!service && !dlg->text().isEmpty()) {
            KRun::run(dlg->text(), list, this);
            return;
        }
    }

    // we have an action with a service, run it!
    bool isSuccess = KRun::runService(*service, list, this, isTempfile);
    Q_ASSERT(isSuccess);
    Q_UNUSED(isSuccess);
}

void KSnapshot::slotPopulateOpenMenu()
{
    QList<QAction *> currentActions = m_openMenu->actions();
    for (auto currentAction: currentActions) {
        m_openMenu->removeAction(currentAction);
    }

    // Cache the list of actions when the menu is first openend.
    // After that always use the cached list
    if (m_sendToActions->actions().isEmpty()) {
        m_sendToActions->retrieveActions();
    }

    for (auto action: m_sendToActions->actions()) {
        m_openMenu->addAction(action);
    }

    m_openMenu->addSeparator();
    KService::Ptr none;
    m_openMenu->addAction(new KSnapshotServiceAction(none,
                          i18n("Other Application..."),
                          this));
}

void KSnapshot::slotRegionGrabbed(const QPixmap &pix)
{
    if (!pix.isNull()) {
        m_snapshot = pix;
        updatePreview();
        m_modified = true;
        refreshCaption();
    }

    if (m_regionGrab) {
        m_regionGrab->deleteLater();
        m_regionGrab = nullptr;
    }

    if (m_freeRegionGrab) {
        m_freeRegionGrab->deleteLater();
        m_freeRegionGrab = nullptr;
    }

    QApplication::restoreOverrideCursor();
    show();
}

void KSnapshot::slotRegionUpdated(const QRect &selection)
{
    m_lastRegion = selection;
}

void KSnapshot::slotFreeRegionUpdated(const QPolygon &selection)
{
    m_lastFreeRegion = selection;
}

void KSnapshot::slotWindowGrabbed(const QPixmap &pix)
{
    if (!pix.isNull()) {
        m_snapshot = pix;
        updatePreview();
        m_modified = true;
        refreshCaption();
    }

    QApplication::restoreOverrideCursor();
    show();
}

void KSnapshot::slotScreenshotReceived(qulonglong handle)
{
#if XCB_XCB_FOUND
    slotWindowGrabbed(PixmapHelperXCB::grabWindow(handle));
#else
    Q_UNUSED(handle)
#endif
}

void KSnapshot::reject()
{
    close();
}

void KSnapshot::closeEvent(QCloseEvent *e)
{
    KConfigGroup conf(KSharedConfig::openConfig(), "GENERAL");
    conf.writeEntry("delay", delay());
    conf.writeEntry("mode", mode());
    conf.writeEntry("includeDecorations", includeDecorations());
    conf.writeEntry("includePointer", includePointer());

    KConfigGroup cg(KSharedConfig::openConfig(), "MainWindow");
    KWindowConfig::saveWindowSize(windowHandle(), cg);

    QUrl url = m_filename;
    url.setPassword(QString());
    conf.writePathEntry("m_filename", url.url());

    conf.sync();
    e->accept();
}

bool KSnapshot::eventFilter(QObject *o, QEvent *e)
{
    if (o == m_grabber && e->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = (QMouseEvent *) e;
        if (QWidget::mouseGrabber() != m_grabber) {
            return false;
        }
        if (me->button() == Qt::LeftButton) {
            performGrab();
        }
    }
    return false;
}

void KSnapshot::updatePreview()
{
    setPreview(m_snapshot);
}

void KSnapshot::grabFullScreen()
{
    const QList<QScreen *> screens = qApp->screens();
    if (screens.isEmpty()) {
        return;
    }

    const QDesktopWidget *desktop = QApplication::desktop();
    m_snapshot = screens.first()->grabWindow(desktop->winId());
    grabPointerImage(0, 0);
}

void KSnapshot::grabCurrentScreen()
{
    //qDebug() << "Desktop Geom2 = " << QApplication::desktop()->geometry();
    const QList<QScreen *> screens = qApp->screens();
    const QDesktopWidget *desktop = QApplication::desktop();

    const int screenId = desktop->screenNumber(QCursor::pos());
    //qDebug() << "Screenid2 = " << screenId;

    const QRect geom = desktop->screenGeometry(screenId);
    //qDebug() << "Geometry2 = " << geom;
    if (screenId < screens.count()) {
        m_snapshot = screens[screenId]->grabWindow(desktop->winId(),
                     geom.x(), geom.y(), geom.width(), geom.height());
        grabPointerImage(geom.x(), geom.y());
    }
}

void KSnapshot::grabRegion()
{
    if (m_regionGrab) {
        return;
    }

    m_regionGrab = new RegionGrabber(m_lastRegion);
    connect(m_regionGrab, &RegionGrabber::regionGrabbed, this, &KSnapshot::slotRegionGrabbed);
    connect(m_regionGrab, &RegionGrabber::regionUpdated, this, &KSnapshot::slotRegionUpdated);

}

void KSnapshot::grabFreeRegion()
{
    if (m_freeRegionGrab) {
        return;
    }

    m_freeRegionGrab = new FreeRegionGrabber(m_lastFreeRegion);
    connect(m_freeRegionGrab, &FreeRegionGrabber::freeRegionGrabbed, this, &KSnapshot::slotRegionGrabbed);
    connect(m_freeRegionGrab, &FreeRegionGrabber::freeRegionUpdated, this, &KSnapshot::slotFreeRegionUpdated);
}

void KSnapshot::grabTimerDone()
{
    if (mode() == Region) {
        grabRegion();
    } else if (mode() == FreeRegion) {
        grabFreeRegion();
    } else {
        performGrab();
    }
}

void KSnapshot::performGrab()
{
    m_grabber->releaseMouse();
    m_grabber->hide();
    m_grabTimer.stop();

    m_title.clear();
    m_windowClass.clear();

    if (mode() == ChildWindow) {
        WindowGrabber wndGrab;
        connect(&wndGrab, &WindowGrabber::windowGrabbed, this, &KSnapshot::slotWindowGrabbed);
        wndGrab.exec();
        QPoint offset = wndGrab.lastWindowPosition();
        grabPointerImage(offset.x(), offset.y());
        qDebug() << "last window position is" << offset;
    } else if (mode() == WindowUnderCursor) {
        if (m_useKwinEffect) {
            // use kwin effect
            QDBusConnection::sessionBus().connect("org.kde.KWin", "/Screenshot",
                                                  "org.kde.kwin.Screenshot", "screenshotCreated",
                                                  this, SLOT(slotScreenshotReceived(qulonglong)));
            QDBusInterface interface("org.kde.KWin", "/Screenshot", "org.kde.kwin.Screenshot");

            int mask = 0;
            if (includeDecorations()) {
                mask |= 1 << 0;
            }

            if (includePointer()) {
                mask |= 1 << 1;
            }

            interface.call("screenshotWindowUnderCursor", mask);
        } else {
            m_snapshot = WindowGrabber::grabCurrent(includeDecorations());

            // If we're showing decorations anyway then we'll add the title and window
            // class to the output image meta data.
            if (includeDecorations()) {
                m_title = WindowGrabber::lastWindowTitle();
                m_windowClass = WindowGrabber::lastWindowClass();
            }

            QPoint offset = WindowGrabber::lastWindowPosition();
            grabPointerImage(offset.x(), offset.y());
        }
    } else if (mode() == CurrentScreen) {
        grabCurrentScreen();
    } else {
        grabFullScreen();
    }

    updatePreview();
    QApplication::restoreOverrideCursor();
    m_modified = true;
    refreshCaption();
    if (m_savedPosition != QPoint(-1, -1)) {
        move(m_savedPosition);
    }
    show();
}

// Grabs the pointer image if there is platform support for it, and overlays it onto the snapshot.
void KSnapshot::grabPointerImage(int offsetx, int offsety)
{
#if XCB_XFIXES_FOUND
    PixmapHelperXCB::compositePointer(offsetx, offsety, m_snapshot);
#else
    Q_UNUSED(offsetx);
    Q_UNUSED(offsety);
    return;
#endif
}

void KSnapshot::setTime(int newTime)
{
    setDelay(newTime);
}

int KSnapshot::timeout() const
{
    return delay();
}

void KSnapshot::setURL(const QString &url)
{
    changeUrl(url);
}

void KSnapshot::setGrabMode(int m)
{
    setMode(m);
}

int KSnapshot::grabMode() const
{
    return mode();
}

void KSnapshot::refreshCaption()
{
    setWindowTitle(m_filename.fileName() + " [*]");
    setWindowModified(m_modified);
}

void KSnapshot::slotMovePointer(int x, int y)
{
    QCursor::setPos(x, y);
}

void KSnapshot::exit()
{
    reject();
}

void KSnapshot::slotModeChanged(int mode)
{
    m_snapshotWidget->cbIncludePointer->setEnabled(!(mode == Region || mode == FreeRegion));
    m_snapshotWidget->lblIncludePointer->setEnabled(!(mode == Region || mode == FreeRegion));
    m_snapshotWidget->cbIncludeDecorations->setEnabled(mode == WindowUnderCursor);
    m_snapshotWidget->lblIncludeDecorations->setEnabled(mode == WindowUnderCursor);
}

void KSnapshot::setPreview(const QPixmap &pm)
{
    m_snapshotWidget->lblImage->setToolTip(
        i18n("Preview of the snapshot image (%1 x %2)" ,
             pm.width(), pm.height()));

    m_snapshotWidget->lblImage->setPreview(pm);
    m_snapshotWidget->lblImage->adjustSize();
}

void KSnapshot::setDelay(int i)
{
    m_snapshotWidget->spinDelay->setValue(i);
}

void KSnapshot::setIncludeDecorations(bool b)
{
    m_snapshotWidget->cbIncludeDecorations->setChecked(b);
}

void KSnapshot::setIncludePointer(bool enabled)
{
    m_snapshotWidget->cbIncludePointer->setChecked(enabled);
}

bool KSnapshot::includePointer() const
{
    return m_snapshotWidget->cbIncludePointer->isChecked();
}

void KSnapshot::setMode(int mode)
{
    m_snapshotWidget->comboMode->setCurrentIndex(mode);
    slotModeChanged(mode);
}

int KSnapshot::delay() const
{
    return m_snapshotWidget->spinDelay->value();
}

bool KSnapshot::includeDecorations() const
{
    return m_snapshotWidget->cbIncludeDecorations->isChecked();
}

int KSnapshot::mode() const
{
    return m_snapshotWidget->comboMode->currentIndex();
}

QPixmap KSnapshot::preview()
{
    return *m_snapshotWidget->lblImage->pixmap();
}

int KSnapshot::previewWidth() const
{
    return m_snapshotWidget->lblImage->width();
}

int KSnapshot::previewHeight() const
{
    return m_snapshotWidget->lblImage->height();
}


