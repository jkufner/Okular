#ifndef _PSGS_H_
#define _PSGS_H_

#include <ktempfile.h>
#include <qstring.h>
#include <qintdict.h>
#include <qintcache.h>
#include <qpixmap.h>
#include <qobject.h>


class pageInfo
{
public:
  pageInfo(QString _PostScriptString);
  ~pageInfo();

  QColor    background;
  QString   *PostScriptString;
  KTempFile *Gfx;
};


// Maximal number of PostScript-Pages which are held in memory (or on
// the disk) for speedup. This should later be made dynamic, maybe
// with the possibility of switching on/off.
#define PAGES_IN_MEMORY_CACHE  13
#define PAGES_IN_DISK_CACHE   101


class ghostscript_interface  : public QObject 
{
 Q_OBJECT

public:
  ghostscript_interface(double dpi, int pxlw, int pxlh);
  ~ghostscript_interface();

  void setSize(double dpi, int pxlw, int pxlh);

  void clear();

  // sets the PostScript which is used on a certain page
  void setPostScript(int page, QString PostScript);

  // sets path from additional postscript files may be read
  void setIncludePath(const QString &_includePath);

  // sets the background color for a certain page
  void setColor(int page, QColor background_color);

  // Returns the graphics of the page, if possible. The functions
  // returns a pointer to a QPixmap, or null. The referred QPixmap
  // should be deleted after use.
  QPixmap  *graphics(int page);

  // Returns the background color for a certain page. If no color was
  // set, Qt::white is returned.
  QColor   getBackgroundColor(int page);

  QString  *PostScriptHeaderString;

private:
  void                  gs_generate_graphics_file(int page, const QString &filename);
  QIntDict<pageInfo>    pageList;

  // Chache to store pages which contain PostScript and are therefore
  // slow to render.
  QIntCache<QPixmap>    MemoryCache;

  // Chache to store pages which contain PostScript and are therefore
  // slow to render.
  QIntCache<KTempFile>  DiskCache;

  double                resolution;   // in dots per inch
  int                   pixel_page_w; // in pixels
  int                   pixel_page_h; // in pixels

  QString               includePath;

  // Output device that ghostscript is supposed tp use. Default is
  // "png256". If that does not work, gs_generate_graphics_file will
  // automatically try other known device drivers. If no known output
  // device can be found, something is badly wrong. In that case,
  // "gsDevice" is set to an empty string, and
  // gs_generate_graphics_file will return immediately.
  QValueListIterator<QString> gsDevice;

  // A list of know devices, set by the constructor. This includes
  // "png256", "pnm". If a device is found to not work, its name is
  // removed from the list, and another device name is tried.
  QStringList           knownDevices;

signals:
  /** Passed through to the top-level kpart. */
  void setStatusBarText( const QString& );
};

#endif
