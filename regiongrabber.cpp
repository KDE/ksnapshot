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
  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include "regiongrabber.h"

#include <qpainter.h>
#include <qpalette.h>
#include <qstyle.h>
#include <qtimer.h>

#include <kglobalsettings.h>

SizeTip::SizeTip( QWidget *parent, const char *name )
    : QLabel( parent, name, WStyle_Customize | WX11BypassWM |
      WStyle_StaysOnTop | WStyle_NoBorder | WStyle_Tool )
{
  setMargin( 2 );
  setIndent( 0 );
  setFrameStyle( QFrame::Plain | QFrame::Box );

  QColorGroup cg(  Qt::black, QColor( 255,255,220 ),
      QColor( 96,96,96 ), Qt::black, Qt::black,
      Qt::black, QColor( 255,255,220 ) );
  QPalette pal(  cg, cg, cg );

  setPalette( pal );
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
  : QLabel( 0, 0, WStyle_Customize | WX11BypassWM ),
    mouseDown( false ), sizeTip( 0L )
{
  sizeTip = new SizeTip( ( QWidget * )0L );

  tipTimer = new QTimer( this );
  connect( tipTimer, SIGNAL( timeout() ), SLOT( updateSizeTip() ) );

  QTimer::singleShot( 200, this, SLOT( initGrabber() ) );
}

RegionGrabber::~RegionGrabber()
{
  delete sizeTip;
}

void RegionGrabber::initGrabber()
{
  pixmap = QPixmap::grabWindow( qt_xrootwin() );
  setPixmap( pixmap );

  showFullScreen();

  grabMouse( crossCursor );
}

void RegionGrabber::mousePressEvent( QMouseEvent *e )
{
  if ( e->button() == LeftButton )
  {
    mouseDown = true;
    grabRect = QRect( e->pos(), e->pos() );
  }
}

void RegionGrabber::mouseMoveEvent( QMouseEvent *e )
{
  if ( mouseDown )
  {
    sizeTip->hide();
    tipTimer->start( 250, true );

    drawRubber();
    grabRect.setBottomRight( e->pos() );
    drawRubber();
  }
}

void RegionGrabber::mouseReleaseEvent( QMouseEvent *e )
{
  mouseDown = false;
  drawRubber();
  sizeTip->hide();

  grabRect.setBottomRight( e->pos() );
  grabRect = grabRect.normalize();

  QPixmap region = QPixmap::grabWindow( winId(), grabRect.x(), grabRect.y(),
      grabRect.width(), grabRect.height() );

  releaseMouse();

  emit regionGrabbed( region );
}

void RegionGrabber::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Key_Escape )
  {
    releaseMouse();
    emit regionGrabbed( QPixmap() );
  }
  else
    e->ignore();
}

void RegionGrabber::updateSizeTip()
{
  QRect rect = grabRect.normalize();

  sizeTip->setTip( rect );
  sizeTip->show();
}

void RegionGrabber::drawRubber()
{
  QPainter p;
  p.begin( this );
  p.setRasterOp( NotROP );
  p.setPen( QPen( color0, 1 ) );
  p.setBrush( NoBrush );

  style().drawPrimitive( QStyle::PE_FocusRect, &p, grabRect, colorGroup(),
      QStyle::Style_Default, QStyleOption( colorGroup().base() ) );

  p.end();
}
