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
#include "kbackgroundsnapshot.moc"
#include "regiongrabber.h"
#include "ksnapshot_options.h"
#include <windowgrabber.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <KAboutData>
#include <KApplication>
#include <KGlobalSettings>
#include <kio/netaccess.h>

#include <QMouseEvent>
#include <QPixmap>
#include <QDesktopWidget>

KBackgroundSnapshot::KBackgroundSnapshot(KSnapshotObject::CaptureMode mode)
	:KSnapshotObject()
{
    modeCapture=mode;
    grabber = new QWidget( 0,  Qt::X11BypassWindowManagerHint );
    grabber->move( -1000, -1000 );
    grabber->installEventFilter( this );
    grabber->show();
    grabber->grabMouse( Qt::WaitCursor );

    if ( mode == KSnapshotObject::FullScreen )
    {
        snapshot = QPixmap::grabWindow( QApplication::desktop()->winId() );
        savePictureOnDesktop();
    }
    else {
	switch(mode)
	{
            case KSnapshotObject::WindowUnderCursor:
	   {
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
    if(mode !=  KSnapshotObject::ChildWindow)
    {
       grabber->releaseMouse();
       grabber->hide();
    }

}

KBackgroundSnapshot::~KBackgroundSnapshot()
{
    //kDebug()<<" KBackgroundSnapshot::~KBackgroundSnapshot()\n";
}

void KBackgroundSnapshot::savePictureOnDesktop()
{
    filename = KUrl( KGlobalSettings::desktopPath()+'/'+i18n("snapshot")+"1.png" );
    // Make sure the name is not already being used
    while(KIO::NetAccess::exists( filename, KIO::NetAccess::DestinationSide, 0L )) {
        autoincFilename();
    }
    save( filename, 0L);
    exit( 0 );
}

void KBackgroundSnapshot::performGrab()
{
    //kDebug()<<"KBackgroundSnapshot::performGrab()\n";
    grabber->releaseMouse();
    grabber->hide();
    if ( modeCapture == ChildWindow ) {
        WindowGrabber wndGrab;
        connect( &wndGrab, SIGNAL(windowGrabbed(QPixmap)),
                           SLOT(slotWindowGrabbed(QPixmap)) );
        wndGrab.exec();
        savePictureOnDesktop();
    }
    else if ( modeCapture == WindowUnderCursor ) {
        snapshot = WindowGrabber::grabCurrent( true );
        savePictureOnDesktop();
    }
    else {
        snapshot = QPixmap::grabWindow( QApplication::desktop()->winId() );
        savePictureOnDesktop();
    }
}

void KBackgroundSnapshot::slotWindowGrabbed( const QPixmap &pix )
{
    //kDebug()<<" KBackgroundSnapshot::slotWindowGrabbed( const QPixmap &pix )\n";
    if ( !pix.isNull() )
        snapshot = pix;
    QApplication::restoreOverrideCursor();
}


void KBackgroundSnapshot::slotGrab()
{
    //kDebug()<<"KBackgroundSnapshot::slotGrab()\n";
    grabber->show();
    grabber->grabMouse( Qt::CrossCursor );
}


void KBackgroundSnapshot::grabRegion()
{
   rgnGrab = new RegionGrabber();
   connect( rgnGrab, SIGNAL(regionGrabbed(QPixmap)),
                     SLOT(slotRegionGrabbed(QPixmap)) );

}


void KBackgroundSnapshot::slotRegionGrabbed( const QPixmap &pix )
{
  if ( !pix.isNull() )
    snapshot = pix;
  rgnGrab->deleteLater();
  QApplication::restoreOverrideCursor();
  savePictureOnDesktop();
}

bool KBackgroundSnapshot::eventFilter( QObject* o, QEvent* e)
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


#define KBACKGROUNDSNAPVERSION "0.1"

static const char description[] = I18N_NOOP("KDE Background Screenshot Utility");

int main(int argc, char **argv)
{
  KAboutData aboutData( "kbackgroundsnapshot", "ksnapshot", ki18n("KBackgroundSnapshot"),
    KBACKGROUNDSNAPVERSION, ki18n(description), KAboutData::License_GPL,
    ki18n("(c) 2007, Montel Laurent"));

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( ksnapshot_options() ); // Add our own options.
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KApplication app;

  if ( args->isSet( "current" ) )
     new KBackgroundSnapshot( KSnapshotObject::WindowUnderCursor );
  else if(args->isSet( "fullscreen" ))
     new KBackgroundSnapshot( KSnapshotObject::FullScreen );
  else if(args->isSet( "region" ))
     new KBackgroundSnapshot( KSnapshotObject::Region );
  else if(args->isSet( "freeregion" ))
     new KBackgroundSnapshot( KSnapshotObject::FreeRegion );
  else if(args->isSet( "child" ))
     new KBackgroundSnapshot( KSnapshotObject::ChildWindow );
  else
     new KBackgroundSnapshot();
  args->clear();
  return app.exec();
}
