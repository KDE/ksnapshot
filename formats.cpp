#include <kimgio.h>
#include "formats.h"
#include "gif.h"
#include "eps.h"

static int numFormats= 5;

  // Constants for flags
const unsigned int FormatRecord::InternalFormat= 1;
const unsigned int FormatRecord::ReadFormat= 2;
const unsigned int FormatRecord::WriteFormat= 4;

static FormatRecord formatlist[]= {
  {
    "JPEG",
    FormatRecord::InternalFormat | FormatRecord::ReadFormat,
    "^\377\330\377\340",
    "*.jpeg *.jpg",
    "jpg",
    0, 0,
  }, 
  {
    "GIF",
    FormatRecord::WriteFormat,
    "^GIF[0-9][0-9][a-z]",
    "*.gif",
    "gif",
    0, write_gif_image,
  },
  {
	 "PS",
	 FormatRecord::WriteFormat,
	 0,
	 "*.ps",
	 "ps",
	 0, write_eps_image,
  },
  {
    "BMP",
    FormatRecord::InternalFormat | FormatRecord::ReadFormat | FormatRecord::WriteFormat,
    0,
    "*.bmp",
    "bmp",
    0, 0,
  },
  {	 
    "XBM",
    FormatRecord::InternalFormat | FormatRecord::ReadFormat | FormatRecord::WriteFormat,
    0,
    "*.xbm",
    "xbm",
    0, 0,
  },
  {
    "XPM",
    FormatRecord::InternalFormat | FormatRecord::ReadFormat | FormatRecord::WriteFormat,
    0,
    "*.xpm",
    "xpm",
    0, 0,
  },
  {
    "PNM",
    0, 0,
    "*.pbm *.pgm *.ppm",
    "ppm",
    0, 0
  }
};

FormatManager::FormatManager()
{
   list.setAutoDelete(TRUE);
   init(formatlist);
}

FormatManager::~FormatManager()
{
}

void FormatManager::init(FormatRecord formatlist[])
{
   int i;
   FormatRecord *rec;
   
   // Build format list
   for (i= 0; i < numFormats; i++) {
     list.append(&formatlist[i]);
     names.append(formatlist[i].formatName);
     globAll.append(formatlist[i].glob);
     if (i < numFormats-1)
       globAll.append(" ");
   };
   
   // Register them with Qt
   for (rec= list.first(); rec != 0; rec= list.next()) {
     if ( (rec->flags & FormatRecord::InternalFormat) == 0L)
       QImageIO::defineIOHandler(rec->formatName, rec->magic,
				 0, 
				 rec->read_format, rec->write_format);
   }
   // Register the ones implemented by kimgio (tiff, jpeg, png, ...)
   kimgioRegister();
}
	  
QStrList *FormatManager::formats(void)
{
  return &names;
}

const char *FormatManager::allImagesGlob(void)
{
  return globAll;
}
 
const char *FormatManager::glob(const char *format)
{
  FormatRecord *rec;
  QString name(format);
  QString curr;
  bool done= FALSE;

  rec= list.first();
  do { 
    curr= rec->formatName;
    if (curr == name)
      done= TRUE;
    else 
      rec= list.next();
  } while (!done && (rec != 0));

  if (done)
    return rec->glob;
  else
    return 0;
}

const char *FormatManager::suffix(const char *format)
{
  FormatRecord *rec;
  QString name(format);
  QString curr;
  bool done= FALSE;

  rec= list.first();
  do { 
    curr= rec->formatName;
    if (curr == name)
      done= TRUE;
    else 
      rec= list.next();
  } while (!done && (rec != 0));

  if (done)
    return rec->suffix;
  else
    return 0;
}

