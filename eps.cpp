#include <qimage.h>
#include <qdstream.h>
#include <qtstream.h>
#include <qdir.h>
#include <qfile.h>
#include <qpainter.h>
#include <time.h>
#include <qprinter.h>
#include <qpdevmet.h>
#include <qstring.h>
#include "eps.h"

// This is a wrapper in case the filter is being used in an app other
// than ksnapshot.
#ifndef KSNAPVERSION
#define KSNAPVERSION
#endif // KSNAPVERSION

void write_eps_image(QImageIO *imageio)
{
  QPrinter	 psOut;
  QPainter	 p;

  // making some definitions (papersize, output to file, filename):
  psOut.setCreator( "KSnapshot " KSNAPVERSION);
  psOut.setOutputToFile( TRUE );

  // Make up a name for the temporary file
  QString tempname;
  tempname.sprintf("/tmp/ksnapshot%i", time( 0L ));
  psOut.setOutputFileName(tempname);

  // painting the pixmap to the "printer" which is a file
  p.begin( &psOut );

  //	 p.translate( 0, 0 );
  QPixmap img;
  img.convertFromImage(imageio->image());

  p.drawPixmap( QPoint( 0, 0 ), img );
  p.end();

  // write BoundingBox to File
  QFile		 inFile(tempname);
  QFile		 outFile(imageio->fileName());
  QString		 szBoxInfo;

  szBoxInfo.sprintf("%sBoundingBox: 0 0 %d %d\n%sEndComments\n",
		    "%%", img.width(),
		    img.height(), "%%" );

  inFile.open( IO_ReadOnly );
  outFile.open( IO_WriteOnly );

  QTextStream in( &inFile );
  QTextStream out( &outFile );
  QString		 szInLine;
  int			 nFilePos;

  do{
    nFilePos = inFile.at();
    szInLine = in.readLine();
    out << szInLine << '\n';
  }
  while( szInLine != "%%EndComments" );

  if( outFile.at( nFilePos ) ){
    out << szBoxInfo;
  }

  while( !in.eof() ){
    szInLine = in.readLine();
    out << szInLine << '\n';
  }

  inFile.close();
  outFile.close();

  QDir dir("/tmp");

  dir.remove( tempname, FALSE );

  imageio->setStatus(0);
}

