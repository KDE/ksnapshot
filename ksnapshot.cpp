/*
 *  Copyright (C) 1997-2002 Richard J. Moore
 *  Copyright (C) 2000 Matthias Ettrich
 *  Copyright (C) 2002 Aaron J. Seigo
 *  Copyright (C) 2003 Nadeem Hasan
 *  Copyright (C) 2004 Bernd Brandstetter
 *  Copyright (C) 2006 Urs Wolfer <uwolfer @ fwo.ch>
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

#include <QClipboard>
#include <QPainter>
#include <QShortcut>

#include <klocale.h>
#include <kimageio.h>
#include <kfiledialog.h>
#include <kimagefilepreview.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kprinter.h>
#include <kio/netaccess.h>
#include <ksavefile.h>
#include <kstdaccel.h>
#include <ktempfile.h>
#include <knotification.h>
#include <khelpmenu.h>
#include <kmenu.h>
#include <kstartupinfo.h>
#include <kvbox.h>
#include <kinstance.h>
#include <kglobal.h>

#include "ksnapshot.h"
#include "regiongrabber.h"
#include "windowgrabber.h"
#include "ui_ksnapshotwidget.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>

class KSnapshotWidget : public QWidget, public Ui::KSnapshotWidget
{
    public:
        KSnapshotWidget(QWidget *parent = 0)
        : QWidget(parent)
        {
            setupUi(this);
        }
};

KSnapshot::KSnapshot(QWidget *parent, bool grabCurrent)
  : KDialog(parent)
{
    setModal( true );
    showButtonSeparator( true );
    setDefaultButton( User1 );
    setButtons(Help|User1);
    setButtonGuiItem( User1, KStdGuiItem::quit() );
    grabber = new QWidget( 0,  Qt::X11BypassWindowManagerHint );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );

    KStartupInfo::appStarted();

    KVBox *vbox = new KVBox( this );
    vbox->setSpacing( spacingHint() );
    setMainWidget( vbox );

    mainWidget = new KSnapshotWidget( vbox );

    connect( mainWidget->lblImage, SIGNAL( startDrag() ), SLOT( slotDragSnapshot() ) );
    connect( mainWidget->btnNew, SIGNAL( clicked() ), SLOT( slotGrab() ) );
    connect( mainWidget->btnSave, SIGNAL( clicked() ), SLOT( slotSaveAs() ) );
    connect( mainWidget->btnPrint, SIGNAL( clicked() ), SLOT( slotPrint() ) );
    connect( mainWidget->btnCopy, SIGNAL( clicked() ), SLOT( slotCopy() ) );
    connect( mainWidget->comboMode, SIGNAL( activated(int) ), SLOT( slotModeChanged(int) ) );

    grabber->show();
    grabber->grabMouse( Qt::WaitCursor );

    if ( !grabCurrent )
        snapshot = QPixmap::grabWindow( QX11Info::appRootWindow() );
    else {
        setMode( WindowUnderCursor );
        setIncludeDecorations( true );
        performGrab();
    }

    grabber->releaseMouse();
    grabber->hide();

    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    setDelay( conf->readEntry("delay", 0) );
    setMode( conf->readEntry("mode", 0) );
    setIncludeDecorations(conf->readEntry("includeDecorations",true));
    filename = KUrl( conf->readPathEntry( "filename", QDir::currentPath()+'/'+i18n("snapshot")+"1.png" ));

    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, false, this )) {
        autoincFilename();
    }

    connect( &grabTimer, SIGNAL( timeout() ), this, SLOT(  grabTimerDone() ) );
    connect( &updateTimer, SIGNAL( timeout() ), this, SLOT(  updatePreview() ) );
    QTimer::singleShot( 0, this, SLOT( updateCaption() ) );

    KHelpMenu *helpMenu = new KHelpMenu(this, KGlobal::instance()->aboutData(), false);
    setButtonMenu( Help, helpMenu->menu() );
#if 0
    accel->insert( "QuickSave", i18n("Quick Save Snapshot &As..."),
                   i18n("Save the snapshot to the file specified by the user without showing the file dialog."),
                   Qt::CTRL+Qt::SHIFT+Qt::Key_S, this, SLOT(slotSave()));
    accel->insert( "SaveAs", i18n("Save Snapshot &As..."),
                   i18n("Save the snapshot to the file specified by the user."),
                   Qt::CTRL+Qt::Key_A, this, SLOT(slotSaveAs()));
#endif

    new QShortcut( KStdAccel::shortcut( KStdAccel::Quit ), this, SLOT(reject()));

    new QShortcut( Qt::Key_Q, this, SLOT(slotSave()));

    new QShortcut( KStdAccel::shortcut( KStdAccel::Copy ), mainWidget->btnCopy, SLOT(animateClick()));

    new QShortcut( KStdAccel::shortcut( KStdAccel::Save ), mainWidget->btnSave, SLOT(animateClick()));
    new QShortcut( Qt::Key_S, mainWidget->btnSave, SLOT(animateClick()));

    new QShortcut( KStdAccel::shortcut( KStdAccel::Print ), mainWidget->btnPrint, SLOT(animateClick()));
    new QShortcut( Qt::Key_P, mainWidget->btnPrint, SLOT(animateClick()));

    new QShortcut( KStdAccel::shortcut( KStdAccel::New ), mainWidget->btnNew, SLOT(animateClick()) );
    new QShortcut( Qt::Key_N, mainWidget->btnNew, SLOT(animateClick()) );
    new QShortcut( Qt::Key_Space, mainWidget->btnNew, SLOT(animateClick()) );

    setEscapeButton( User1 );
    connect( this, SIGNAL( user1Clicked() ), SLOT( reject() ) );

    mainWidget->btnNew->setFocus();
}

KSnapshot::~KSnapshot()
{
    delete grabber;
    delete mainWidget;
}

void KSnapshot::resizeEvent( QResizeEvent * )
{
    updateTimer.setSingleShot( true );
    updateTimer.start( 200 );
}

bool KSnapshot::save( const QString &filename )
{
    return save( KUrl( filename ));
}

bool KSnapshot::save( const KUrl& url )
{
    if ( KIO::NetAccess::exists( url, false, this ) ) {
        const QString title = i18n( "File Exists" );
        const QString text = i18n( "<qt>Do you really want to overwrite <b>%1</b>?</qt>" , url.prettyUrl());
        if (KMessageBox::Continue != KMessageBox::warningContinueCancel( this, text, title, i18n("Overwrite") ) )
        {
            return false;
        }
    }

    QByteArray type = "PNG";
    QString mime = KMimeType::findByURL( url.fileName(), 0, url.isLocalFile(), true )->name();
    QStringList types = KImageIO::typeForMime(mime);
    if ( !types.isEmpty() )
        type = types.first().toLatin1();

    bool ok = false;

    if ( url.isLocalFile() ) {
        KSaveFile saveFile( url.path() );
        if ( saveFile.status() == 0 ) {
            if ( snapshot.save( saveFile.file(), type ) )
                ok = saveFile.close();
        }
    }
    else {
    KTempFile tmpFile;
        tmpFile.setAutoDelete( true );
        if ( tmpFile.status() == 0 ) {
            if ( snapshot.save( tmpFile.file(), type ) ) {
                if ( tmpFile.close() )
                    ok = KIO::NetAccess::upload( tmpFile.name(), url, this );
            }
        }
    }

    QApplication::restoreOverrideCursor();
    if ( !ok ) {
        kWarning() << "KSnapshot was unable to save the snapshot" << endl;

        QString caption = i18n("Unable to save image");
        QString text = i18n("KSnapshot was unable to save the image to\n%1.", url.prettyUrl());
        KMessageBox::error(this, text, caption);
    }

    return ok;
}

void KSnapshot::slotSave()
{
    if ( save(filename) ) {
        modified = false;
        autoincFilename();
    }
}

void KSnapshot::slotSaveAs()
{
    QStringList mimetypes = KImageIO::mimeTypes( KImageIO::Writing );
    KFileDialog dlg( filename.url(), mimetypes.join(" "), this);

    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setCaption( i18n("Save As") );

    KImageFilePreview *ip = new KImageFilePreview( &dlg );
    dlg.setPreviewWidget( ip );

    if ( !dlg.exec() )
        return;

    KUrl url = dlg.selectedUrl();
    if ( !url.isValid() )
        return;

    if ( save(url) ) {
        filename = url;
        modified = false;
        autoincFilename();
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
    hide();

    if ( delay() ) {
        grabTimer.setSingleShot( true );
        grabTimer.start( delay() * 1000 );
    }
    else {
        if ( mode() == Region ) {
            rgnGrab = new RegionGrabber();
            connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
                              SLOT( slotRegionGrabbed( const QPixmap & ) ) );
        }
        else {
            grabber->show();
            grabber->grabMouse( Qt::CrossCursor );
        }
    }
}

void KSnapshot::slotPrint()
{
    KPrinter printer;
    if (snapshot.width() > snapshot.height())
        printer.setOrientation(KPrinter::Landscape);
    else
        printer.setOrientation(KPrinter::Portrait);

    qApp->processEvents();

    if (printer.setup(this, i18n("Print Screenshot")))
    {
        qApp->processEvents();

        QPainter painter(&printer);

        float w = snapshot.width();
        float dw = w - printer.width();
        float h = snapshot.height();
        float dh = h - printer.height();
        bool scale = false;

        if ( (dw > 0.0) || (dh > 0.0) )
            scale = true;

        if ( scale ) {

            QImage img = snapshot.toImage();
            qApp->processEvents();

            float newh, neww;
            if ( dw > dh ) {
                neww = w-dw;
                newh = neww/w*h;
            }
            else {
                newh = h-dh;
                neww = newh/h*w;
            }

            img = img.scaled( int(neww), int(newh), Qt::KeepAspectRatio, Qt::SmoothTransformation );
            qApp->processEvents();

            int x = (printer.width()-img.width())/2;
            int y = (printer.height()-img.height())/2;

            painter.drawImage( x, y, img);
        }
        else {
            int x = (printer.width()-snapshot.width())/2;
            int y = (printer.height()-snapshot.height())/2;
            painter.drawPixmap( x, y, snapshot );
        }
    }

    qApp->processEvents();
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

  delete rgnGrab;
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

void KSnapshot::closeEvent( QCloseEvent * e )
{
    KConfig *conf=KGlobal::config();
    conf->setGroup("GENERAL");
    conf->writeEntry("delay",delay());
    conf->writeEntry("mode",mode());
    conf->writeEntry("includeDecorations",includeDecorations());
    KUrl url = filename;
    url.setPass( QString::null );
    conf->writePathEntry("filename",url.url());
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

void KSnapshot::autoincFilename()
{
    // Extract the filename from the path
    QString name= filename.fileName();

    // If the name contains a number then increment it
    QRegExp numSearch("[0-9]+");

    // Does it have a number?
    int start = numSearch.indexIn(name);
    if (start != -1) {
        // It has a number, increment it
        int len = numSearch.matchedLength();
        QString numAsStr= name.mid(start, len);
        QString number = QString::number(numAsStr.toInt() + 1);
        number = number.rightJustified( len, '0');
        name.replace(start, len, number );
    }
    else {
        // no number
        start = name.lastIndexOf('.');
        if (start != -1) {
            // has a . somewhere, e.g. it has an extension
            name.insert(start, '1');
        }
        else {
            // no extension, just tack it on to the end
            name += '1';
        }
    }

    //Rebuild the path
    KUrl newURL = filename;
    newURL.setFileName( name );
    setURL( newURL.url() );
}

void KSnapshot::updatePreview()
{
    setPreview( snapshot );
}

void KSnapshot::grabTimerDone()
{
    if ( mode() == Region ) {
        rgnGrab = new RegionGrabber();
        connect( rgnGrab, SIGNAL( regionGrabbed( const QPixmap & ) ),
                          SLOT( slotRegionGrabbed( const QPixmap & ) ) );
    }
    else {
        performGrab();
    }
    KNotification::beep(i18n("The screen has been successfully grabbed."));
}

void KSnapshot::performGrab()
{
    grabber->releaseMouse();
    grabber->hide();
    grabTimer.stop();
    if ( mode() == ChildWindow ) {
        WindowGrabber wndGrab;
        connect( &wndGrab, SIGNAL( windowGrabbed( const QPixmap & ) ),
                           SLOT( slotWindowGrabbed( const QPixmap & ) ) );
        wndGrab.exec();
    }
    else if ( mode() == WindowUnderCursor ) {
        snapshot = WindowGrabber::grabCurrent( includeDecorations() );
    }
    else {
        snapshot = QPixmap::grabWindow( QX11Info::appRootWindow() );
    }
    updatePreview();
    QApplication::restoreOverrideCursor();
    modified = true;
    updateCaption();
    show();
}

void KSnapshot::setTime(int newTime)
{
    setDelay(newTime);
}

int KSnapshot::timeout()
{
    return delay();
}

void KSnapshot::setURL( const QString &url )
{
    KUrl newURL = KUrl( url );
    if ( newURL == filename )
        return;

    filename = newURL;
    updateCaption();
}

void KSnapshot::setGrabMode( int m )
{
    setMode( m );
}

int KSnapshot::grabMode()
{
    return mode();
}

void KSnapshot::updateCaption()
{
    KInstance::CaptionFlags flags = KInstance::ModifiedCaption;
    flags |= KInstance::AppNameCaption;
    setCaption( KInstance::makeStdCaption( filename.fileName(), flags) );
}

void KSnapshot::slotMovePointer(int x, int y)
{
    QCursor::setPos( x, y );
}

void KSnapshot::exit()
{
    reject();
}

void KSnapshot::slotModeChanged( int mode )
{
    switch ( mode )
    {
    case 0:
        mainWidget->cbIncludeDecorations->setEnabled(false);
        break;
    case 1:
        mainWidget->cbIncludeDecorations->setEnabled(true);
        break;
    case 2:
        mainWidget->cbIncludeDecorations->setEnabled(false);
        break;
    case 3:
        mainWidget->cbIncludeDecorations->setEnabled(false);
        break;
    default:
        break;
    }
}

void KSnapshot::setPreview( const QPixmap &pm )
{
    QPixmap pmScaled = pm.scaled( previewWidth(), previewHeight(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    mainWidget->lblImage->setToolTip(
        i18n( "Preview of the snapshot image (%1 x %2)" ,
          pm.width(), pm.height() ) );

    mainWidget->lblImage->setPixmap( pmScaled );
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

void KSnapshot::setMode( int mode )
{
    mainWidget->comboMode->setCurrentIndex(mode);
    slotModeChanged(mode);
}

int KSnapshot::delay()
{
    return mainWidget->spinDelay->value();
}

bool KSnapshot::includeDecorations()
{
    return mainWidget->cbIncludeDecorations->isChecked();
}

int KSnapshot::mode()
{
    return mainWidget->comboMode->currentIndex();
}

QPixmap KSnapshot::preview()
{
    return *mainWidget->lblImage->pixmap();
}

int KSnapshot::previewWidth()
{
    return mainWidget->lblImage->width();
}

int KSnapshot::previewHeight()
{
    return mainWidget->lblImage->height();
}

#include "ksnapshot.moc"
