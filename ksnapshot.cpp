/*
 *  Copyright (C) 1997-2008 Richard J. Moore <rich@kde.org>
 *  Copyright (C) 2000 Matthias Ettrich <ettrich@troll.no>
 *  Copyright (C) 2002 Aaron J. Seigo <aseigo@kde.org>
 *  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>
 *  Copyright (C) 2004 Bernd Brandstetter <bbrand@freenet.de>
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ kde.org>
 *  Copyright (C) 2010 Martin Gräßlin <kde@martin-graesslin.com>
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

#include <QClipboard>
#include <QShortcut>
#include <QMenu>
#include <QDesktopWidget>
#include <QVarLengthArray>
#include <QtCore/QXmlStreamReader>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#include <QtDBus/QDBusInterface>

#include <klocale.h>

#include <KDebug>
#include <kglobal.h>
#include <kicon.h>
#include <kimageio.h>
#include <kcomponentdata.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <ksavefile.h>
#include <kstandardshortcut.h>
#include <ktemporaryfile.h>
#include <knotification.h>
#include <khelpmenu.h>
#include <kmenu.h>
#include <kmimetypetrader.h>
#include <kopenwithdialog.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <kstartupinfo.h>
#include <kvbox.h>
#include <qdebug.h>

#include "regiongrabber.h"
#include "windowgrabber.h"
#include "ksnapshotpreview.h"
#include "ui_ksnapshotwidget.h"

#include "config-ksnapshot.h"

#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
#include <X11/extensions/Xfixes.h>
#include <QX11Info>
#endif

class KSnapshotWidget : public QWidget, public Ui::KSnapshotWidget
{
    public:
        KSnapshotWidget(QWidget *parent = 0)
        : QWidget(parent)
        {
            setupUi(this);
            btnNew->setIcon(KIcon("ksnapshot"));
        }
};

KSnapshot::KSnapshot(QWidget *parent,  KSnapshotObject::CaptureMode mode )
  : KDialog(parent), KSnapshotObject(), modified(false), savedPosition(QPoint(-1, -1))
{
    setCaption( "" );
    setModal( true );
    showButtonSeparator( true );
    setButtons(Help | Apply | User1 | User2);
    setButtonGuiItem(Apply, KStandardGuiItem::saveAs());
    setButtonGuiItem(User1, KGuiItem(i18n("Copy"), "edit-copy"));
    setButtonGuiItem(User2, KGuiItem(i18n("Open With..."), "document-open"));
    setDefaultButton(Apply);
    grabber = new QWidget( 0,  Qt::X11BypassWindowManagerHint );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    KStartupInfo::appStarted();

    KVBox *vbox = new KVBox( this );
    vbox->setSpacing( spacingHint() );
    setMainWidget( vbox );

    mainWidget = new KSnapshotWidget( vbox );

    connect(mainWidget->lblImage, SIGNAL(startDrag()), SLOT(slotDragSnapshot()));
    connect(mainWidget->btnNew, SIGNAL(clicked()), SLOT(slotGrab()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotSaveAs()));
    connect(this, SIGNAL(user1Clicked()), SLOT(slotCopy()));
    connect(mainWidget->comboMode, SIGNAL(activated(int)), SLOT(slotModeChanged(int)));

    if (qApp->desktop()->numScreens() < 2) {
        mainWidget->comboMode->removeItem(CurrentScreen);
    }

    openMenu = new QMenu(this);
    button(User2)->setMenu(openMenu);
    connect(openMenu, SIGNAL(aboutToShow()), this, SLOT(slotPopulateOpenMenu()));
    connect(openMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotOpen(QAction*)));

    mainWidget->spinDelay->setSuffix(ki18np(" second", " seconds"));

    grabber->show();
    grabber->grabMouse();

    KConfigGroup conf(KGlobal::config(), "GENERAL");

#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
    {
        int tmp1, tmp2;
        //Check whether the XFixes extension is available
        Display *dpy = QX11Info::display();
        if (!XFixesQueryExtension( dpy, &tmp1, &tmp2 )) {
            mainWidget->cbIncludePointer->hide();
            mainWidget->lblIncludePointer->hide();
        }
    }
#else
    mainWidget->cbIncludePointer->hide();
    mainWidget->lblIncludePointer->hide();
#endif
    setIncludePointer(conf.readEntry("includePointer", false));

    // check if kwin screenshot effect is available
    includeAlpha = false;
    if ( QDBusConnection::sessionBus().interface()->isServiceRegistered( "org.kde.kwin" ) ) {
        QDBusInterface kwinInterface( "org.kde.kwin", "/", "org.freedesktop.DBus.Introspectable" );
        QDBusReply<QString> reply = kwinInterface.call( "Introspect" );
        if ( reply.isValid() ) {
            QXmlStreamReader xml( reply.value() );
            while ( !xml.atEnd() ) {
                xml.readNext();
                if ( xml.tokenType() == QXmlStreamReader::StartElement &&
                    xml.name().toString() == "node" ) {
                    if ( xml.attributes().value( "name" ).toString() == "Screenshot" ) {
                        includeAlpha = true;
                        break;
                    }
                }
            }
        }
    }

    kDebug() << "Mode = " << mode;
    if ( mode == KSnapshotObject::FullScreen ) {
        snapshot = QPixmap::grabWindow( QApplication::desktop()->winId() );
#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
        if ( haveXFixes && includePointer() )
            grabPointerImage(0, 0);
#endif
    }
    else if ( mode == KSnapshotObject::CurrentScreen ) {
        kDebug() << "Desktop Geom = " << QApplication::desktop()->geometry();
        QDesktopWidget *desktop = QApplication::desktop();
        int screenId = desktop->screenNumber( QCursor::pos() );
        kDebug() << "Screenid = " << screenId;
        QRect geom = desktop->screenGeometry( screenId );
        kDebug() << "Geometry = " << screenId;
        snapshot = QPixmap::grabWindow( desktop->winId(),
                geom.x(), geom.y(), geom.width(), geom.height() );
    }
    else {
        setMode( mode );
        switch(mode)
        {
            case KSnapshotObject::WindowUnderCursor:
                {
                    setIncludeDecorations( true );
                    performGrab();
                    break;
                }
            case  KSnapshotObject::ChildWindow:
                {
                    slotGrab();
                    break;
                }
            case KSnapshotObject::Region:
                {
                    grabRegion();
                    break;
                }
            default:
                break;
        }
    }

    //When we use argument to take snapshot we mustn't hide it.
    if (mode !=  KSnapshotObject::ChildWindow) {
       grabber->releaseMouse();
       grabber->hide();
    }

    setDelay( conf.readEntry("delay", 0) );
    setMode( conf.readEntry("mode", 0) );
    setIncludeDecorations(conf.readEntry("includeDecorations",true));
    filename = KUrl( conf.readPathEntry( "filename", QDir::currentPath()+'/'+i18n("snapshot")+"1.png" ));

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    connect( &updateTimer, SIGNAL( timeout() ), this, SLOT(  updatePreview() ) );
    QTimer::singleShot( 0, this, SLOT( updateCaption() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::mainComponent().aboutData(), true);
    setButtonMenu( Help, helpMenu->menu() );
#if 0
    accel->insert( "QuickSave", i18n("Quick Save Snapshot &As..."),
                   i18n("Save the snapshot to the file specified by the user without showing the file dialog."),
                   Qt::CTRL+Qt::SHIFT+Qt::Key_S, this, SLOT(slotSave()));
    accel->insert( "SaveAs", i18n("Save Snapshot &As..."),
                   i18n("Save the snapshot to the file specified by the user."),
                   Qt::CTRL+Qt::Key_A, this, SLOT(slotSaveAs()));
#endif

    new QShortcut( KStandardShortcut::shortcut( KStandardShortcut::Quit ).primary(), this, SLOT(reject()));

    new QShortcut( Qt::Key_Q, this, SLOT(slotSave()));

    new QShortcut( KStandardShortcut::shortcut( KStandardShortcut::Copy ).primary(), button(User1), SLOT(animateClick()));

    new QShortcut( KStandardShortcut::shortcut( KStandardShortcut::Save ).primary(), button(Apply), SLOT(animateClick()));
    new QShortcut( Qt::Key_S, button(Apply), SLOT(animateClick()));

    new QShortcut( KStandardShortcut::shortcut( KStandardShortcut::New ).primary(), mainWidget->btnNew, SLOT(animateClick()) );
    new QShortcut( Qt::Key_N, mainWidget->btnNew, SLOT(animateClick()) );
    new QShortcut( Qt::Key_Space, mainWidget->btnNew, SLOT(animateClick()) );

    connect( this, SIGNAL( user1Clicked() ), SLOT( reject() ) );

    mainWidget->btnNew->setFocus();
    setInitialSize(QSize(250, 500));

    KConfigGroup cg(KGlobal::config(), "MainWindow");
    restoreDialogSize(cg);
}

KSnapshot::~KSnapshot()
{
    delete mainWidget;
}

void KSnapshot::resizeEvent( QResizeEvent * )
{
    updateTimer.setSingleShot( true );
    updateTimer.start( 200 );
}

void KSnapshot::slotSave()
{
    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, KIO::NetAccess::DestinationSide, this )) {
        autoincFilename();
    }

    if ( save(filename, this) ) {
        modified = false;
        autoincFilename();
        updateCaption();
    }
}

void KSnapshot::slotSaveAs()
{
    QStringList mimetypes = KImageIO::mimeTypes( KImageIO::Writing );

    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, KIO::NetAccess::DestinationSide, this )) {
        autoincFilename();
    }

    KFileDialog dlg( filename.url(), mimetypes.join(" "), this);

    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );
    dlg.setSelection( filename.url() );

    if ( !dlg.exec() )
        return;

    KUrl url = dlg.selectedUrl();
    if ( !url.isValid() )
        return;

    if ( save(url,this) ) {
        filename = url;
        modified = false;
        autoincFilename();
        updateCaption();
    }
}

void KSnapshot::slotCopy()
{
    QClipboard *cb = QApplication::clipboard();
    cb->setPixmap( snapshot );
}

void KSnapshot::slotDragSnapshot()
{
    QDrag *drag = new QDrag(this);

    drag->setMimeData(new QMimeData);
    drag->mimeData()->setImageData(snapshot);
    drag->setPixmap(preview());
    drag->start();
}

void KSnapshot::slotGrab()
{
    savedPosition = pos();
    hide();

    if (delay()) {
        //kDebug() << "starting timer with time of" << delay();
        grabTimer.start(delay());
    }
    else {
        QTimer::singleShot(0, this, SLOT(startUndelayedGrab()));
    }
}

void KSnapshot::startUndelayedGrab()
{
    if (mode() == Region) {
        grabRegion();
    }
    else {
        grabber->show();
        grabber->grabMouse(Qt::CrossCursor);
    }
}

KUrl KSnapshot::urlToOpen(bool *isTempfile)
{
    if (isTempfile) {
        *isTempfile = false;
    }

    if (!modified && filename.isValid())
    {
        return filename;
    }

    const QString fileopen = KStandardDirs::locateLocal("tmp", filename.fileName());

    if (saveEqual(fileopen, this))
    {
        if (isTempfile) {
            *isTempfile = true;
        }

        return fileopen;
    }

    return KUrl();
}

void KSnapshot::slotOpen(const QString& application)
{
    KUrl url = urlToOpen();
    if (!url.isValid())
    {
        return;
    }

    KUrl::List list;
    list.append(url);
    KRun::run(application, list, this);
}

void KSnapshot::slotOpen(QAction* action)
{
    KSnapshotServiceAction* serviceAction =
                                  qobject_cast<KSnapshotServiceAction*>(action);

    if (!serviceAction)
    {
        return;
    }

    bool isTempfile = false;
    KUrl url = urlToOpen(&isTempfile);
    if (!url.isValid())
    {
        return;
    }

    KUrl::List list;
    list.append(url);

    KService::Ptr service = serviceAction->service;
    if (!service)
    {
        KOpenWithDialog dlg(list, this);
        if (!dlg.exec())
        {
            return;
        }

        service = dlg.service();

        if (!service && !dlg.text().isEmpty())
        {
             KRun::run(dlg.text(), list, this);
             return;
        }
    }

    // we have an action with a service, run it!
    KRun::run(*service, list, this, isTempfile);
}

void KSnapshot::slotPopulateOpenMenu()
{
    QList<QAction*> currentActions = openMenu->actions();
    foreach (QAction* currentAction, currentActions)
    {
        openMenu->removeAction(currentAction);
        currentAction->deleteLater();
    }

    const KService::List services = KMimeTypeTrader::self()->query( "image/png");
    QMap<QString, KService::Ptr> apps;

    foreach (const KService::Ptr &service, services)
    {
        apps.insert(service->name(), service);
    }

    foreach (const KService::Ptr &service, apps)
    {
        QString name = service->name().replace( "&", "&&" );
        openMenu->addAction(new KSnapshotServiceAction(service,
                                                       KIcon(service->icon()),
                                                       name, this));
    }

    openMenu->addSeparator();
    KService::Ptr none;
    openMenu->addAction(new KSnapshotServiceAction(none,
                                                   i18n("Other Application..."),
                                                   this));
}

void KSnapshot::slotRegionGrabbed( const QPixmap &pix )
{
  if ( !pix.isNull() )
  {
    snapshot = pix;
    updatePreview();
    modified = true;
    updateCaption();
  }

  rgnGrab->deleteLater();
  QApplication::restoreOverrideCursor();
  show();
}

void KSnapshot::slotWindowGrabbed( const QPixmap &pix )
{
    if ( !pix.isNull() )
    {
        snapshot = pix;
        updatePreview();
        modified = true;
        updateCaption();
    }

    QApplication::restoreOverrideCursor();
    show();
}

void KSnapshot::slotScreenshotReceived( qulonglong handle )
{
    slotWindowGrabbed( QPixmap::fromX11Pixmap( handle ) );
}

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfigGroup conf(KGlobal::config(), "GENERAL");
    conf.writeEntry("delay", delay());
    conf.writeEntry("mode", mode());
    conf.writeEntry("includeDecorations", includeDecorations());
    conf.writeEntry("includePointer", includePointer());

    KConfigGroup cg(KGlobal::config(), "MainWindow");
    saveDialogSize(cg);

    KUrl url = filename;
    url.setPass(QString::null); //krazy:exclude=nullstrassign for old broken gcc
    conf.writePathEntry("filename", url.url());

    conf.sync();
    e->accept();
}

bool KSnapshot::eventFilter( QObject* o, QEvent* e)
{
    if ( o == grabber && e->type() == QEvent::MouseButtonPress ) {
        QMouseEvent* me = (QMouseEvent*) e;
        if ( QWidget::mouseGrabber() != grabber )
            return false;
        if ( me->button() == Qt::LeftButton )
            performGrab();
    }
    return false;
}

void KSnapshot::updatePreview()
{
    setPreview( snapshot );
}

void KSnapshot::grabRegion()
{
   rgnGrab = new RegionGrabber();
   connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
                     SLOT( slotRegionGrabbed( const QPixmap & ) ) );

}

void KSnapshot::grabTimerDone()
{
    if ( mode() == Region ) {
        grabRegion();
    }
    else {
        performGrab();
    }
    KNotification::beep(i18n("The screen has been successfully grabbed."));
}

void KSnapshot::performGrab()
{
    int x = 0;
    int y = 0;

    grabber->releaseMouse();
    grabber->hide();
    grabTimer.stop();

    title.clear();
    windowClass.clear();

    if ( mode() == ChildWindow ) {
        WindowGrabber wndGrab;
        connect( &wndGrab, SIGNAL( windowGrabbed( const QPixmap & ) ),
                           SLOT( slotWindowGrabbed( const QPixmap & ) ) );
        wndGrab.exec();
        QPoint offset = wndGrab.lastWindowPosition();
        x = offset.x();
        y = offset.y();
        qDebug() << "last window position is" << offset;
    }
    else if ( mode() == WindowUnderCursor ) {
        if ( includeAlpha ) {
            // use kwin effect
            QDBusConnection::sessionBus().connect("org.kde.kwin", "/Screenshot",
                                                  "org.kde.kwin.Screenshot", "screenshotCreated",
                                                  this, SLOT(slotScreenshotReceived(qulonglong)));
            QDBusInterface interface( "org.kde.kwin", "/Screenshot", "org.kde.kwin.Screenshot" );
            int mask = 0;
            if ( includeDecorations() )
            {
                mask |= 1 << 0;
            }
            if ( includePointer() )
            {
                mask |= 1 << 1;
            }
            interface.call( "screenshotWindowUnderCursor", mask );
        } else {
            snapshot = WindowGrabber::grabCurrent( includeDecorations() );

            QPoint offset = WindowGrabber::lastWindowPosition();
            x = offset.x();
            y = offset.y();

            // If we're showing decorations anyway then we'll add the title and window
            // class to the output image meta data.
            if ( includeDecorations() ) {
                title = WindowGrabber::lastWindowTitle();
                windowClass = WindowGrabber::lastWindowClass();
            }
        }
    }
    else if ( mode() == CurrentScreen ) {
        kDebug() << "Desktop Geom2 = " << QApplication::desktop()->geometry();
        QDesktopWidget *desktop = QApplication::desktop();
        int screenId = desktop->screenNumber( QCursor::pos() );
        kDebug() << "Screenid2 = " << screenId;
        QRect geom = desktop->screenGeometry( screenId );
        kDebug() << "Geometry2 = " << geom;
        x = geom.x();
        y = geom.y();
        snapshot = QPixmap::grabWindow( desktop->winId(),
                x, y, geom.width(), geom.height() );
    }
    else {
        snapshot = QPixmap::grabWindow( QApplication::desktop()->winId() );
    }
#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
    if (haveXFixes && includePointer()) {
        grabPointerImage(x, y);
    }
#endif

    updatePreview();
    QApplication::restoreOverrideCursor();
    modified = true;
    updateCaption();
    if (savedPosition != QPoint(-1, -1)) {
        move(savedPosition);
    }
    show();
}

void KSnapshot::grabPointerImage(int offsetx, int offsety)
// Uses the X11_EXTENSIONS_XFIXES_H extension to grab the pointer image, and overlays it onto the snapshot.
{
#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
    XFixesCursorImage *xcursorimg = XFixesGetCursorImage( QX11Info::display() );
    if ( !xcursorimg )
      return;

    //Annoyingly, xfixes specifies the data to be 32bit, but places it in an unsigned long *
    //which can be 64 bit.  So we need to iterate over a 64bit structure to put it in a 32bit
    //structure.
    QVarLengthArray< quint32 > pixels( xcursorimg->width * xcursorimg->height );
    for (int i = 0; i < xcursorimg->width * xcursorimg->height; ++i)
        pixels[i] = xcursorimg->pixels[i] & 0xffffffff;

    QImage qcursorimg((uchar *) pixels.data(), xcursorimg->width, xcursorimg->height,
                       QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&snapshot);
    painter.drawImage(QPointF(xcursorimg->x - xcursorimg->xhot - offsetx, xcursorimg->y - xcursorimg ->yhot - offsety), qcursorimg);

    XFree(xcursorimg);
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

void KSnapshot::setURL( const QString &url )
{
    changeUrl( url );
}

void KSnapshot::setGrabMode( int m )
{
    setMode( m );
}

int KSnapshot::grabMode() const
{
    return mode();
}

void KSnapshot::refreshCaption()
{
    updateCaption();
}

void KSnapshot::updateCaption()
{
    setCaption( filename.fileName(), modified );
}

void KSnapshot::slotMovePointer(int x, int y)
{
    QCursor::setPos( x, y );
}

void KSnapshot::exit()
{
    reject();
}

void KSnapshot::slotModeChanged(int mode)
{
    mainWidget->cbIncludePointer->setEnabled(mode != Region);
    mainWidget->lblIncludePointer->setEnabled(mode != Region);
    mainWidget->cbIncludeDecorations->setEnabled(mode == WindowUnderCursor);
    mainWidget->lblIncludeDecorations->setEnabled(mode == WindowUnderCursor);
}

void KSnapshot::setPreview( const QPixmap &pm )
{
    mainWidget->lblImage->setToolTip(
        i18n( "Preview of the snapshot image (%1 x %2)" ,
          pm.width(), pm.height() ) );

    mainWidget->lblImage->setPreview(pm);
    mainWidget->lblImage->adjustSize();
}

void KSnapshot::setDelay( int i )
{
    mainWidget->spinDelay->setValue(i);
}

void KSnapshot::setIncludeDecorations( bool b )
{
    mainWidget->cbIncludeDecorations->setChecked(b);
}

void KSnapshot::setIncludePointer(bool enabled)
{
    mainWidget->cbIncludePointer->setChecked(enabled);
}

bool KSnapshot::includePointer() const
{
    return mainWidget->cbIncludePointer->isChecked();
}

void KSnapshot::setMode( int mode )
{
    mainWidget->comboMode->setCurrentIndex(mode);
    slotModeChanged(mode);
}

int KSnapshot::delay() const
{
    return mainWidget->spinDelay->value();
}

bool KSnapshot::includeDecorations() const
{
    return mainWidget->cbIncludeDecorations->isChecked();
}

int KSnapshot::mode() const
{
    return mainWidget->comboMode->currentIndex();
}

QPixmap KSnapshot::preview()
{
    return *mainWidget->lblImage->pixmap();
}

int KSnapshot::previewWidth() const
{
    return mainWidget->lblImage->width();
}

int KSnapshot::previewHeight() const
{
    return mainWidget->lblImage->height();
}

#include "ksnapshot.moc"
