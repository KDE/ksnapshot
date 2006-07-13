/*
  Copyright (C) 2003 Nadeem Hasan <nhasan@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or ( at your option ) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; see the file COPYING.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "regiongrabber.h"

#include <QApplication>
#include <QTimer>
#include <QToolTip>
#include <QX11Info>
#include <QRubberBand>

#include <kglobalsettings.h>

SizeTip::SizeTip( QWidget *parent )
    : QLabel( parent, Qt::X11BypassWindowManagerHint |
      Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool )
{
  setMargin( 2 );
  setIndent( 0 );
  setFrameStyle( QFrame::Plain | QFrame::Box );

  setPalette( QToolTip::palette() );
}

void SizeTip::setTip( const QRect &rect )
{
  QString tip = QString( "%1x%2" ).arg( rect.width() )
      .arg( rect.height() );

  setText( tip );
  adjustSize();

  positionTip( rect );
}

void SizeTip::positionTip( const QRect &rect )
{
  QRect tipRect = geometry();
  tipRect.moveTopLeft( QPoint( 0, 0 ) );

  if ( rect.intersects( tipRect ) )
  {
    QRect deskR = KGlobalSettings::desktopGeometry( QPoint( 0, 0 ) );

    tipRect.moveCenter( QPoint( deskR.width()/2, deskR.height()/2 ) );
    if ( !rect.contains( tipRect, true ) && rect.intersects( tipRect ) )
      tipRect.moveBottomRight( geometry().bottomRight() );
  }

  move( tipRect.topLeft() );
}

RegionGrabber::RegionGrabber()
  : QWidget( 0, Qt::X11BypassWindowManagerHint |
      Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool ),
    mouseDown( false ), sizeTip( 0L )
{
  sizeTip = new SizeTip( ( QWidget * )0L );
  band    = new QRubberBand( QRubberBand::Rectangle, this );

  tipTimer = new QTimer( this );
  tipTimer->setInterval( 250 );
  tipTimer->setSingleShot( true );
  connect( tipTimer, SIGNAL( timeout() ), SLOT( updateSizeTip() ) );

  QTimer::singleShot( 200, this, SLOT( initGrabber() ) );
}

RegionGrabber::~RegionGrabber()
{
  delete sizeTip;
}

void RegionGrabber::initGrabber()
{
  pixmap = QPixmap::grabWindow( QX11Info::appRootWindow() );
  QPalette p = palette();
  p.setBrush( backgroundRole(), QBrush( pixmap ) );
  setPalette( p );

  showFullScreen();

  QApplication::setOverrideCursor( Qt::CrossCursor );
}

void RegionGrabber::mousePressEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    mouseDown = true;
    grabRect = QRect( e->pos(), e->pos() );
    band->setGeometry( grabRect );
    band->show();
  }
}

void RegionGrabber::mouseMoveEvent( QMouseEvent *e )
{
  if ( mouseDown )
  {
    sizeTip->hide();
    tipTimer->start();

    grabRect.setBottomRight( e->pos() );
    band->setGeometry( grabRect.normalized() );
  }
}

void RegionGrabber::mouseReleaseEvent( QMouseEvent *e )
{
  mouseDown = false;
  band->hide();
  sizeTip->hide();

  grabRect.setBottomRight( e->pos() );
  grabRect = grabRect.normalized();

  QPixmap region = QPixmap::grabWindow( winId(), grabRect.x(), grabRect.y(),
      grabRect.width(), grabRect.height() );

  QApplication::restoreOverrideCursor();

  emit regionGrabbed( region );
}

void RegionGrabber::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    QApplication::restoreOverrideCursor();
    emit regionGrabbed( QPixmap() );
  }
  else
    e->ignore();
}

void RegionGrabber::updateSizeTip()
{
  sizeTip->setTip( grabRect.normalized() );
  sizeTip->show();
}

#include "regiongrabber.moc"
