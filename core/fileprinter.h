/***************************************************************************
 *   Copyright (C) 2007, 2010 by John Layt <john@layt.net>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// This Class is a temporary addition to Okular for the duration of KDE 4.0.
// In KDE 4.1 this class will either be moved to kdelibs if still required,
// or replaced  with a Qt 4.4 based solution.

#ifndef FILEPRINTER_H
#define FILEPRINTER_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QPrinter>

// For KDE_DEPRECATED
#include <kdemacros.h>

#include "okular_export.h"
#include "generator.h"

class QSize;

namespace Okular {

class OKULAR_EXPORT FilePrinter
{
public:

    /** Whether file(s) get deleted by the application or by the print system.
     *
     *  You may need to chose system deletion if your temp file clean-up
     *  deletes the file before the print system is finished with it.
     */
    enum FileDeletePolicy { ApplicationDeletesFiles, SystemDeletesFiles };

    /** Whether pages to be printed are selected by the application or the print system.
     *
     *  If application side, then the generated file will only contain those pages
     *  selected by the user, so FilePrinter will print all the pages in the file.
     *
     *  If system side, then the file will contain all the pages in the document, and
     *  the print system will print the users selected print range from out of the file.
     *
     *  Note system side only works in CUPS, not LPR.
     */
    enum PageSelectPolicy { ApplicationSelectsPages, SystemSelectsPages };

    /** Print a file using the settings in QPrinter
     *
     *  Only supports CUPS and LPR on *NIX.  Page Range only supported in CUPS.
     *  Most settings unsupported by LPR, some settings unsupported by CUPS.
     *
     *  The documentOrientation parameter was added in version 0.14.
     *
     * @param printer the print settings to use
     * @param file the file to print
     * @param documentOrientation the orientation stored in the document itself
     * @param fileDeletePolicy if the application or system deletes the file
     * @param pageSelectPolicy if the application or system selects the pages to print
     * @param pageRange page range to print if SystemSlectsPages and user chooses Selection in Print Dialog
     *
     * @returns Returns exit code:
     *          -9 if lpr not found
     *          -8 if empty file name
     *          -7 if unable to find file
     *          -6 if invalid printer state
     *          -5 if print to file copy failed
     *          -2 if the KProcess could not be started
     *          -1 if the KProcess crashed
     *          otherwise the KProcess exit code
     *
     * @since 0.14 (KDE 4.8)
     */

    static int printFile( QPrinter &printer, const QString file,
                          QPrinter::Orientation documentOrientation,
                          FileDeletePolicy fileDeletePolicy = FilePrinter::ApplicationDeletesFiles,
                          PageSelectPolicy pageSelectPolicy = FilePrinter::ApplicationSelectsPages,
                          const QString &pageRange = QString() );

    /** Print a file using the settings in QPrinter (compatibility overload)
     *
     *  Only supports CUPS and LPR on *NIX.  Page Range only supported in CUPS.
     *  Most settings unsupported by LPR, some settings unsupported by CUPS.
     *
     * @param printer the print settings to use
     * @param file the file to print
     * @param fileDeletePolicy if the application or system deletes the file
     * @param pageSelectPolicy if the application or system selects the pages to print
     * @param pageRange page range to print if SystemSlectsPages and user chooses Selection in Print Dialog
     *
     * @returns Returns exit code:
     *          -9 if lpr not found
     *          -8 if empty file name
     *          -7 if unable to find file
     *          -6 if invalid printer state
     *          -5 if print to file copy failed
     *          -2 if the KProcess could not be started
     *          -1 if the KProcess crashed
     *          otherwise the KProcess exit code
     *
     * @deprecated Use the overload which takes the documentOrientation instead.
     */

    static KDE_DEPRECATED int printFile( QPrinter &printer, const QString file,
                                         FileDeletePolicy fileDeletePolicy = FilePrinter::ApplicationDeletesFiles,
                                         PageSelectPolicy pageSelectPolicy = FilePrinter::ApplicationSelectsPages,
                                         const QString &pageRange = QString() );

    /** Print a list of files using the settings in QPrinter
     *
     *  Only supports CUPS and LPR on *NIX.
     *  Most settings unsupported by LPR, some settings unsupported by CUPS.
     *
     * @param printer the print settings to use
     * @param fileList the files to print
     * @param fileDeletePolicy if the application or system deletes the file
     *
     * @returns Returns exit code:
     *          -9 if lpr not found
     *          -8 if empty file list
     *          -7 if unable to find a file
     *          -6 if invalid printer state
     *          -5 if print to file copy failed
     *          -2 if the KProcess could not be started
     *          -1 if the KProcess crashed
     *          otherwise the KProcess exit code
     *
     * @deprecated Use printFile instead, passing the documentOrientation for each file.
     */

    static KDE_DEPRECATED int printFiles( QPrinter &printer, const QStringList &fileList,
                                          FileDeletePolicy fileDeletePolicy = FilePrinter::ApplicationDeletesFiles );

    /** Return the list of pages selected by the user in the Print Dialog
     *
     * @param printer the print settings to use
     * @param lastPage the last page number, needed if AllPages option is selected
     * @param currentPage the current page number, needed if CurrentPage option is selected
     * @param selectedPageList list of pages to use if Selection option is selected
     * @returns Returns list of pages to print
     */
    static QList<int> pageList( QPrinter &printer, int lastPage,
                                int currentPage, const QList<int> &selectedPageList );

    /** Return the list of pages selected by the user in the Print Dialog
     *
     * @param printer the print settings to use
     * @param lastPage the last page number, needed if AllPages option is selected
     * @param selectedPageList list of pages to use if Selection option is selected
     * @returns Returns list of pages to print
     */
    static QList<int> pageList( QPrinter &printer, int lastPage, const QList<int> &selectedPageList );

    /** Return the range of pages selected by the user in the Print Dialog
     *
     * @param printer the print settings to use
     * @param lastPage the last page number, needed if AllPages option is selected
     * @param selectedPageList list of pages to use if Selection option is selected
     * @returns Returns range of pages to print
     */
    static QString pageRange( QPrinter &printer, int lastPage, const QList<int> &selectedPageList );

    /** convert a Page List into a Page Range
     *
     * @param pageList list of pages to convert
     * @returns Returns equivalent page range
     */
    static QString pageListToPageRange( const QList<int> &pageList );

    /** Return if Ghostscript ps2pdf is available on this system
     *
     * @returns Returns true if Ghostscript ps2pdf available
     */
     static bool ps2pdfAvailable();

    /** Return if Ghostscript pdf2ps is available on this system
     *
     * @returns Returns true if Ghostscript pdf2ps available
     */
     static bool pdf2psAvailable();

    /** Return if CUPS Print System is available on this system
     *
     * @returns Returns true if CUPS available
     */
    static bool cupsAvailable();

    /** Returns the postscript standard page size
     *
     * @returns Returns paper size in ps points
     */
    static QSize psPaperSize( QPrinter &printer );

    /**
     * Convert the code returned by printFile* to PrintError
     * @since 0.11 (KDE 4.5)
     */
    static Generator::PrintError printError( int c );

protected:

    bool detectCupsService();
    bool detectCupsConfig();

    KDE_DEPRECATED int doPrintFiles( QPrinter &printer, const QStringList fileList,
                                     FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy,
                                     const QString &pageRange );
    int doPrintFiles( QPrinter &printer, const QStringList fileList,
                             FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy,
                             const QString &pageRange,
                             QPrinter::Orientation documentOrientation );

    KDE_DEPRECATED QStringList printArguments( QPrinter &printer,
                                               FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy,
                                               bool useCupsOptions, const QString &pageRange, const QString &version );
    QStringList printArguments( QPrinter &printer,
                                       FileDeletePolicy fileDeletePolicy, PageSelectPolicy pageSelectPolicy,
                                       bool useCupsOptions, const QString &pageRange, const QString &version,
                                       QPrinter::Orientation documentOrientation );

    QStringList destination( QPrinter &printer, const QString &version );
    QStringList copies( QPrinter &printer, const QString &version );
    QStringList jobname( QPrinter &printer, const QString &version );
    QStringList deleteFile( QPrinter &printer, FileDeletePolicy fileDeletePolicy,
                                   const QString &version );
    QStringList pages( QPrinter &printer, PageSelectPolicy pageSelectPolicy,
                              const QString &pageRange, bool useCupsOptions, const QString &version );

    KDE_DEPRECATED QStringList cupsOptions( QPrinter &printer );
    QStringList cupsOptions( QPrinter &printer, QPrinter::Orientation documentOrientation );
    QStringList optionMedia( QPrinter &printer );
    QString mediaPageSize( QPrinter &printer );
    QString mediaPaperSource( QPrinter &printer );
    KDE_DEPRECATED QStringList optionOrientation( QPrinter &printer );
    QStringList optionOrientation( QPrinter &printer, QPrinter::Orientation documentOrientation );
    QStringList optionDoubleSidedPrinting( QPrinter &printer );
    QStringList optionPageOrder( QPrinter &printer );
    QStringList optionCollateCopies( QPrinter &printer );
    QStringList optionPageMargins( QPrinter &printer );
    QStringList optionCupsProperties( QPrinter &printer );
};

}

#endif // FILEPRINTER_H
