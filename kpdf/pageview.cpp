/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *   Copyright (C) 2004 by Albert Astals Cid <tsdgeos@terra.es>            *
 *                                                                         *
 *   With portions of code from kpdf_pagewidget.cc by:                     *
 *     Copyright (C) 2002 by Wilco Greven <greven@kde.org>                 *
 *     Copyright (C) 2003 by Christophe Devriese                           *
 *                           <Christophe.Devriese@student.kuleuven.ac.be>  *
 *     Copyright (C) 2003 by Laurent Montel <montel@kde.org>               *
 *     Copyright (C) 2003 by Dirk Mueller <mueller@kde.org>                *
 *     Copyright (C) 2004 by James Ots <kde@jamesots.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <qcursor.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qpushbutton.h>

#include <kiconloader.h>
#include <kurldrag.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kconfigbase.h>

#include <math.h>

#include "pageview.h"
#include "pixmapwidget.h"
#include "page.h"


// structure used internally by PageView for data storage
class PageViewPrivate
{
public:
    // the document, current page and pages indices vector
    KPDFDocument * document;
    PageWidget * page; //equal to pages[vectorIndex]
    QValueVector< PageWidget * > pages;
    int vectorIndex;

    // view layout, zoom and mouse
    int viewColumns;
    bool viewContinous;
    PageView::ZoomMode zoomMode;
    float zoomFactor;
    PageView::MouseMode mouseMode;
    QPoint mouseGrabPos;
    QPoint mouseStartPos;
    bool mouseOnLink;

    // other stuff
    QTimer *delayTimer;
    bool dirty;

    // actions
    KSelectAction *aZoom;
    KToggleAction *aZoomFitWidth;
    KToggleAction *aZoomFitPage;
    KToggleAction *aZoomFitText;
    KToggleAction *aViewTwoPages;
    KToggleAction *aViewContinous;
};


/*
 * PageView class
 */
PageView::PageView( QWidget *parent, KPDFDocument *document )
    : QScrollView( parent, "KPDF::pageView", WNoAutoErase | WStaticContents )
{
    // create and initialize private storage structure
    d = new PageViewPrivate();
    d->document = document;
    d->page = 0;
    d->viewColumns = 1;
    d->viewContinous = false;
    d->zoomMode = ZoomFitWidth;
    d->zoomFactor = 0.999;
    d->mouseMode = MouseNormal;
    d->mouseOnLink = false;
    d->delayTimer = 0;
    d->dirty = false;

    // dealing with (very) large areas so enable clipper
    enableClipper( true );

    // widget setup: setup focus, accept drops and track mouse
    viewport()->setFocusProxy( this );
    viewport()->setFocusPolicy( StrongFocus );
    viewport()->setPaletteBackgroundColor( Qt::gray );
    setResizePolicy( Manual );
    setAcceptDrops( true );
    viewport()->setMouseTracking( true );

    // conntect the padding of the viewport to pixmaps requests
    connect( this, SIGNAL(contentsMoving(int, int)), this, SLOT(slotRequestVisiblePixmaps(int, int)) );

    // set a corner button to resize the view to the page size
    QPushButton * resizeButton = new QPushButton( viewport() );
    resizeButton->setPixmap( SmallIcon("crop") );
    setCornerWidget( resizeButton );
    resizeButton->setEnabled( false );
    // connect(...);
}

PageView::~PageView()
{
    delete d;
}

void PageView::setupActions( KActionCollection * ac, KConfigGroup * config )
{
    // Zoom actions ( higher scales consumes lots of memory! )
    const double zoomValue[10] = { 0.125, 0.25, 0.333, 0.5, 0.667, 0.75, 1, 1.25, 1.50, 2 };

    d->aZoom = new KSelectAction( i18n( "Zoom" ), "viewmag", 0, ac, "zoom_to" );
    connect( d->aZoom, SIGNAL( activated( const QString & ) ), this, SLOT( slotZoom( const QString& ) ) );
    d->aZoom->setEditable(  true );

    QStringList translated;
    translated << i18n("Fit Width") << i18n("Fit Page");
    QString localValue;
    QString double_oh( "00" );
    for ( int i = 0; i < 10; i++ )
    {
        localValue = KGlobal::locale()->formatNumber( zoomValue[i] * 100.0, 2 );
        localValue.remove( KGlobal::locale()->decimalSymbol() + double_oh );
        translated << QString( "%1%" ).arg( localValue );
    }
    d->aZoom->setItems( translated );
    d->aZoom->setCurrentItem( 0 ); // 8 for 100%

    KStdAction::zoomIn( this, SLOT( slotZoomIn() ), ac, "zoom_in" );

    KStdAction::zoomOut( this, SLOT( slotZoomOut() ), ac, "zoom_out" );

    d->aZoomFitWidth = new KToggleAction( i18n("Fit to Page &Width"), "viewmagfit", 0, ac, "zoom_fit_width" );
    connect( d->aZoomFitWidth, SIGNAL( toggled( bool ) ), SLOT( slotFitToWidthToggled( bool ) ) );

    d->aZoomFitPage = new KToggleAction( i18n("Fit to &Page"), "viewmagfit", 0, ac, "zoom_fit_page" );
    connect( d->aZoomFitPage, SIGNAL( toggled( bool ) ), SLOT( slotFitToPageToggled( bool ) ) );

    // View-Layout actions
    d->aViewTwoPages = new KToggleAction( i18n("Two Pages"), "view_left_right", 0, ac, "view_twopages" );
    connect( d->aViewTwoPages, SIGNAL( toggled( bool ) ), SLOT( slotTwoPagesToggled( bool ) ) );
    d->aViewTwoPages->setChecked( config->readBoolEntry( "ViewTwoPages", false ) );
    slotTwoPagesToggled( d->aViewTwoPages->isChecked() );

    d->aViewContinous = new KToggleAction( i18n("Continous"), "view_text", 0, ac, "view_continous" );
    connect( d->aViewContinous, SIGNAL( toggled( bool ) ), SLOT( slotContinousToggled( bool ) ) );
    d->aViewContinous->setChecked( config->readBoolEntry( "ViewContinous", true ) );
    slotContinousToggled( d->aViewContinous->isChecked() );

    // Mouse-Mode actions
    KToggleAction * mn = new KToggleAction( i18n("Normal"), "mouse", 0, this, SLOT( slotSetMouseNormal() ), ac, "mouse_drag" );
    mn->setExclusiveGroup("MouseType");
    mn->setChecked( true );

    KToggleAction * ms = new KToggleAction( i18n("Select"), "frame_edit", 0, this, SLOT( slotSetMouseSelect() ), ac, "mouse_select" );
    ms->setExclusiveGroup("MouseType");
    ms->setEnabled( false ); // implement feature before removing this line

    KToggleAction * md = new KToggleAction( i18n("Draw"), "edit", 0, this, SLOT( slotSetMouseDraw() ), ac, "mouse_draw" );
    md->setExclusiveGroup("MouseType");
    md->setEnabled( false ); // implement feature before removing this line

    // Other actions
    KToggleAction * ss = new KToggleAction( i18n( "Show &Scrollbars" ), 0, ac, "show_scrollbars" );
    ss->setCheckedState(i18n("Hide &Scrollbars"));
    connect( ss, SIGNAL( toggled( bool ) ), SLOT( slotToggleScrollBars( bool ) ) );

    ss->setChecked( config->readBoolEntry( "ShowScrollBars", true ) );
    slotToggleScrollBars( ss->isChecked() );
}

void PageView::saveSettings( KConfigGroup * config )
{
    config->writeEntry( "ShowScrollBars", hScrollBarMode() == AlwaysOn );
    config->writeEntry( "ViewTwoPages", d->aViewTwoPages->isChecked() );
    config->writeEntry( "ViewContinous", d->aViewContinous->isChecked() );
}


//BEGIN KPDFDocumentObserver inherited methods 
void PageView::pageSetup( const QValueVector<KPDFPage*> & pageSet, bool /*documentChanged*/ )
{ /* TODO: preserve (reuse) existing pages if !documentChanged */
    // delete all pages
    QValueVector< PageWidget * >::iterator dIt = d->pages.begin(), dEnd = d->pages.end();
    for ( ; dIt != dEnd; ++dIt )
        delete *dIt;
    d->pages.clear();
    d->page = 0;

    // create children widgets
    QValueVector< KPDFPage * >::const_iterator setIt = pageSet.begin(), setEnd = pageSet.end();
    for ( ; setIt != setEnd; ++setIt )
    {
        PageWidget * p = new PageWidget( viewport(), *setIt );
        p->setFocusProxy( this );
        d->pages.push_back( p );
    }

    // invalidate layout
    d->dirty = true;
}

void PageView::pageSetCurrent( int pageNumber, float position )
{
    if ( d->dirty )
        reLayoutPages();

    // select next page
    d->vectorIndex = 0;
    d->page = 0;

    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            d->page = *pIt;
            break;
        }
        d->vectorIndex ++;
    }

    if ( d->page )
    {
        int xPos = childX( d->page ) + d->page->widthHint() / 2,
            yPos = childY( d->page ) + (int)((float)d->page->heightHint() * position);
        center( xPos, yPos + visibleHeight() / 2 - 10 );
        slotRequestVisiblePixmaps();
    }
}

void PageView::notifyPixmapChanged( int pageNumber )
{
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
        if ( (*pIt)->pageNumber() == pageNumber )
        {
            (*pIt)->update();
            break;
        }
}
//END KPDFDocumentObserver inherited methods

//BEGIN widget events 
void PageView::contentsMousePressEvent( QMouseEvent * e )
{
    switch ( d->mouseMode )
    {
    case MouseNormal:    // drag / click start
        if ( e->button() & LeftButton )
        {
            d->mouseGrabPos = e->globalPos();
            d->mouseStartPos = d->mouseGrabPos;
            setCursor( sizeAllCursor );
        }
        else if ( e->button() & RightButton )
            emit rightClick();

        /* TODO Albert
            note: 'Page' is an 'information container' and has to deal with clean
            data (such as the '(int)page & (float)position' where link refers to,
            not a LinkAction struct.. better an own struct). 'Document' is the place
            to put xpdf/Splash dependant stuff and fill up pages with interpreted
            data. I think is a *clean* way to handle everything.
            d->pressedLink = *PAGE* or *DOCUMENT* ->findLink( normalizedX, normY );
        */
        break;

    case MouseSelection: // ? set 1st corner of the selection rect ?

    case MouseEdit:      // ? place the beginning of [tool] ?
        break;
    }
}

void PageView::contentsMouseReleaseEvent( QMouseEvent * e )
{
    PageWidget * page = pickPageOnPoint( e->x(), e->y() );
    switch ( d->mouseMode )
    {
    case MouseNormal:    // end drag / follow link
        if ( e->button() & LeftButton )
        {
            setCursor( arrowCursor );
            // check if it was a click, in that case select the page
            if ( e->globalPos() == d->mouseStartPos && page )
                d->document->slotSetCurrentPage( page->pageNumber() );
        }
        else if ( e->button() == Qt::RightButton && page )
        {
            // If over a page display a popup menu
            //FIXME ADD BOOKMARKING STUFF IN DOCUMENT !!!!!!!!!
            KPDFPage * kpdfPage = (KPDFPage *)d->document->page( page->pageNumber() );
            KPopupMenu * m_popup = new KPopupMenu( this, "rmb popup" );
            m_popup->insertTitle( i18n( "Page %1" ).arg( page->pageNumber() ) );
            if ( kpdfPage->isBookmarked() )
                m_popup->insertItem( SmallIcon("bookmark"), i18n("Remove Bookmark"), 1 );
            else
                m_popup->insertItem( SmallIcon("bookmark"), i18n("Add Bookmark"), 1 );
            m_popup->insertItem( SmallIcon("viewmagfit"), i18n("Fit Page"), 2 );
            m_popup->insertItem( SmallIcon("pencil"), i18n("Edit"), 3 );
            switch ( m_popup->exec(QCursor::pos()) )
            {
            case 1:
                kpdfPage->bookmark( !kpdfPage->isBookmarked() );
                break;
            case 2:
                slotTwoPagesToggled( false );
                slotFitToWidthToggled( true );
                d->document->slotSetCurrentPage( page->pageNumber() );
                break;
            case 3:
                //TODO switch to edit
                break;
            }
        }
        /* TODO Albert
            PageLink * link = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
            if ( link == d->pressedLink )
                //go to link, use:
                document->slotSetCurrentPagePosition( (int)link->page(), (float)link->position() );
                //and all the views will update and display the right page at the right position
            d->pressedLink = 0;
        */
        break;

    case MouseSelection: // ? d->page->setPixmapOverlaySelection( QRect ) ?

    case MouseEdit:      // ? apply [tool] ?
        break;
    }
}

void PageView::contentsMouseMoveEvent( QMouseEvent * e )
{
    switch ( d->mouseMode )
    {
    case MouseNormal:    // move page / change mouse cursor if over links
        if ( e->state() & LeftButton )
        {
            QPoint delta = d->mouseGrabPos - e->globalPos();
            scrollBy( delta.x(), delta.y() );
            d->mouseGrabPos = e->globalPos();
        }
        /* TODO Albert
            LinkAction* action = *PAGE* ->findLink(e->x()/m_ppp, e->y()/m_ppp);
            setCursor(action != 0 ? );
            experimental version using Page->hasLink( int pageX, int pageY )
            and haslink has a fake true response on
        */
        // EROS FIXME find right page for query
        /*if ( d->page && e->state() == NoButton &&  )
        {
            bool onLink = d->page->hasLink( e->x() - d->pageRect.left(), e->y() - d->pageRect.top() );
            // set cursor only when entering / leaving (setCursor has not an internal cache)
            if ( onLink != d->mouseOnLink )
            {
                d->mouseOnLink = onLink;
                setCursor( onLink ? pointingHandCursor : arrowCursor );
            }
        }
        */
        break;

    case MouseSelection: // ? update selection contour ?

    case MouseEdit:      // ? update graphics ?
        break;
    }
}

void PageView::viewportResizeEvent( QResizeEvent * )
{
    // start a timer that will refresh the pixmap after 0.5s
    if ( !d->delayTimer )
    {
        d->delayTimer = new QTimer( this );
        connect( d->delayTimer, SIGNAL( timeout() ), this, SLOT( slotUpdateView() ) );
    }
    d->delayTimer->start( 400, true );
}

void PageView::keyPressEvent( QKeyEvent * e )
{
    switch ( e->key() )
    {
    case Key_Up:
        if ( atTop() )
            scrollUp();
        else
            verticalScrollBar()->subtractLine();
        break;
    case Key_Down:
        if ( atBottom() )
            scrollDown();
        else
            verticalScrollBar()->addLine();
        break;
    case Key_Left:
        horizontalScrollBar()->subtractLine();
        break;
    case Key_Right:
        horizontalScrollBar()->addLine();
        break;
    case Key_PageUp:
        verticalScrollBar()->subtractPage();
        break;
    case Key_PageDown:
        verticalScrollBar()->addPage();
        break;
    default:
        e->ignore();
        return;
    }
    e->accept();
}

void PageView::wheelEvent( QWheelEvent *e )
{
    int delta = e->delta();
    e->accept();
    if ( (e->state() & ControlButton) == ControlButton ) {
        if ( e->delta() > 0 )
            slotZoomOut();
        else
            slotZoomIn();
    }
    else if ( delta <= -120 && atBottom() && !d->viewContinous )
        scrollDown();
    else if ( delta >= 120 && atTop() && !d->viewContinous )
        scrollUp();
    else
        QScrollView::wheelEvent( e );
}

void PageView::dragEnterEvent( QDragEnterEvent * ev )
{
    ev->accept();
}

void PageView::dropEvent( QDropEvent * ev )
{
    KURL::List lst;
    if (  KURLDrag::decode(  ev, lst ) )
        emit urlDropped( lst.first() );
}

//END widget events

//BEGIN internal SLOTS
void PageView::slotZoom( const QString & nz )
{
    if ( nz == i18n("Fit Width") )
    {
        d->aZoomFitWidth->setChecked( true );
        return slotFitToWidthToggled( true );
    }
    if ( nz == i18n("Fit Page") )
    {
        d->aZoomFitPage->setChecked( true );
        return slotFitToPageToggled( true );
    }

    QString z = nz;
    z.remove( z.find( '%' ), 1 );
    bool isNumber = true;
    double zoom = KGlobal::locale()->readNumber(  z, &isNumber ) / 100;

    if ( d->zoomFactor != zoom && zoom > 0.1 && zoom < 8.0 )
    {
        d->zoomMode = ZoomFixed;
        d->zoomFactor = zoom;
        slotUpdateView();
        d->aZoomFitWidth->setChecked( false );
        d->aZoomFitPage->setChecked( false );
    }
}

void PageView::slotZoomIn()
{
    if ( d->zoomFactor >= 4.0 )
        return;
    d->zoomFactor += 0.1;
    if ( d->zoomFactor >= 4.0 )
        d->zoomFactor = 4.0;

    d->zoomMode = ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
    d->aZoomFitPage->setChecked( false );
}

void PageView::slotZoomOut()
{
    if ( d->zoomFactor <= 0.125 )
        return;
    d->zoomFactor -= 0.1;
    if ( d->zoomFactor <= 0.125 )
        d->zoomFactor = 0.125;

    d->zoomMode = ZoomFixed;
    slotUpdateView();
    d->aZoomFitWidth->setChecked( false );
    d->aZoomFitPage->setChecked( false );
}

void PageView::slotFitToWidthToggled( bool on )
{
    d->zoomMode = on ? ZoomFitWidth : ZoomFixed;
    slotUpdateView();
    d->aZoomFitPage->setChecked( false );
    //FIXME uncheck others (such as FitToText)
}

void PageView::slotFitToPageToggled( bool on )
{
    ZoomMode newZoomMode = on ? ZoomFitText : ZoomFixed;
    if ( newZoomMode != d->zoomMode )
    {
        d->zoomMode = newZoomMode;
        slotUpdateView();
        d->aZoomFitWidth->setChecked( false );
    }
}

void PageView::slotFitToTextToggled( bool on )
{
    ZoomMode newZoomMode = on ? ZoomFitText : ZoomFixed;
    if ( newZoomMode != d->zoomMode )
    {
        d->zoomMode = newZoomMode;
        slotUpdateView();
        d->aZoomFitWidth->setChecked( false );
    }
}

void PageView::slotTwoPagesToggled( bool on )
{
    int newColumns = on ? 2 : 1;
    if ( d->viewColumns != newColumns )
    {
        d->viewColumns = newColumns;
        reLayoutPages();
    }
}

void PageView::slotContinousToggled( bool on )
{
    if ( d->viewContinous != on )
    {
        d->viewContinous = on;
        reLayoutPages();
    }
}

void PageView::slotSetMouseNormal()
{
    d->mouseMode = MouseNormal;
}

void PageView::slotSetMouseSelect()
{
    d->mouseMode = MouseSelection;
}

void PageView::slotSetMouseDraw()
{
    d->mouseMode = MouseEdit;
}

void PageView::slotToggleScrollBars( bool on )
{
    setHScrollBarMode( on ? AlwaysOn : AlwaysOff );
    setVScrollBarMode( on ? AlwaysOn : AlwaysOff );
}

void PageView::slotUpdateView( bool /*repaint*/ )
{   //TODO ASYNC autogeneration!
    reLayoutPages();
}

void PageView::slotRequestVisiblePixmaps( int newLeft, int newTop )
{
//    // if an update is already scheduled or the widget is hidden, don't proceed
//     if ( (m_delayTimer && m_delayTimer->isActive()) || !isShown() )
//         return;

    // precalc view limits for intersecting with page coords inside the lOOp
    int vLeft = (newLeft == -1) ? contentsX() : newLeft,
        vRight = vLeft + visibleWidth(),
        vTop = (newTop == -1) ? contentsY() : newTop,
        vBottom = vTop + visibleHeight();

    // scroll from the top to the last visible thumbnail
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        int pLeft = childX( p ),
            pRight = pLeft + p->widthHint(),
            pTop = childY( p ),
            pBottom = pTop + p->heightHint();
        if ( p->isShown() && pRight > vLeft && pLeft < vRight && pBottom > vTop && pTop < vBottom )
            d->document->requestPixmap( PAGEVIEW_ID, p->pageNumber(), p->pixmapWidth(), p->pixmapHeight(), true );
    }
}
//END internal SLOTS

void PageView::reLayoutPages()
{
    // set an empty container if we have no pages
    int pageCount = d->pages.count();
    if ( pageCount < 1 )
    {
        resizeContents( 0,0 );
        return;
    }

    int viewportWidth = clipper()->width(),
        viewportHeight = clipper()->height();

    if ( d->viewContinous == TRUE )
    {
        // Here we find out column's width and row's height to compute a table
        // so we can place widgets 'centered in virtual cells'.
        int nCols = d->viewColumns,
            nRows = (int)ceilf( (float)pageCount / (float)nCols ),
            * colWidth = new int[ nCols ],
            * rowHeight = new int[ nRows ],
            cIdx = 0,
            rIdx = 0;
        for ( int i = 0; i < nCols; i++ )
            colWidth[ i ] = viewportWidth / nCols;
        for ( int i = 0; i < nRows; i++ )
            rowHeight[ i ] = 0;

        // 1) find the maximum columns width and rows height for a grid in
        // which each page must well-fit inside a cell
        QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
        for ( ; pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            // update internal page geometry
            if ( d->zoomMode == ZoomFixed )
                p->setZoomFixed( d->zoomFactor );
            else if ( d->zoomMode == ZoomFitWidth )
                p->setZoomFitWidth( colWidth[ cIdx ] - 10 );
            else
                p->setZoomFitRect( colWidth[ cIdx ] - 10, viewportHeight - 10 );
            // find row's maximum height and column's max width
            int pWidth = p->widthHint(),
                pHeight = p->heightHint();
            if ( pWidth > colWidth[ cIdx ] )
                colWidth[ cIdx ] = pWidth;
            if ( pHeight > rowHeight[ rIdx ] )
                rowHeight[ rIdx ] = pHeight;
            // update col/row indices
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
            }
        }

        // 2) arrange widgets inside cells
        int insertX = 0,
            insertY = (int)(5.0 + 10.0 * d->zoomFactor);
        cIdx = 0;
        rIdx = 0;
        for ( pIt = d->pages.begin(); pIt != pEnd; ++pIt )
        {
            PageWidget * p = *pIt;
            int pWidth = p->widthHint(),
                pHeight = p->heightHint(),
                cWidth = colWidth[ cIdx ],
                rHeight = rowHeight[ rIdx ];
            // show, resize and center widget inside 'cells'
            p->resize( pWidth, pHeight );
            moveChild( p, insertX + (cWidth - pWidth) / 2,
                          insertY + (rHeight - pHeight) / 2 );
            p->show();
            // advance col/row index
            insertX += cWidth;
            if ( ++cIdx == nCols )
            {
                cIdx = 0;
                rIdx++;
                insertX = 0;
                insertY += rHeight + (int)(5.0 + 15.0 * d->zoomFactor);
            }
        }

        // 3) update scrollview's contents size and recenter view
        int fullWidth = 0,
            fullHeight = cIdx ? (insertY + rowHeight[ rIdx ] + 10) : insertY,
            oldWidth = contentsWidth(),
            oldHeight = contentsHeight();
        for ( int i = 0; i < nCols; i++ )
            fullWidth += colWidth[ i ];
        resizeContents( fullWidth, fullHeight );
        if ( oldWidth > 0 && oldHeight > 0 )
            center( fullWidth * (contentsX() + visibleWidth() / 2) / oldWidth,
                    fullHeight * (contentsY() + visibleHeight() / 2) / oldHeight );
        else
            center( fullWidth / 2, 0 );

        delete [] colWidth;
        delete [] rowHeight;
    }
    else // viewContinous is FALSE
    {
        // hide all widgets except the displayable ones
        QValueVector< PageWidget * >::iterator dIt = d->pages.begin(), dEnd = d->pages.end();
        for ( ; dIt != dEnd; ++dIt )
            (*dIt)->hide();
        //
        resizeContents( viewportWidth, viewportHeight );
    }

    // reset dirty state
    d->dirty = false;
}

PageWidget * PageView::pickPageOnPoint( int x, int y )
{
    PageWidget * page = 0;
    QValueVector< PageWidget * >::iterator pIt = d->pages.begin(), pEnd = d->pages.end();
    for ( ; pIt != pEnd; ++pIt )
    {
        PageWidget * p = *pIt;
        int pLeft = childX( p ),
            pRight = pLeft + p->widthHint(),
            pTop = childY( p ),
            pBottom = pTop + p->heightHint();
        // little optimized, stops if found or probably quits on the next row
        if ( x > pLeft && x < pRight && y < pBottom )
        {
            if ( y > pTop )
                page = p;
            break;
        }
    }
    return page;
}

bool PageView::atTop() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->minValue();
}

bool PageView::atBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maxValue();
}

void PageView::scrollUp()
{
    if( atTop() && d->vectorIndex > 0 )
        // go to the bottom of previous page
        d->document->slotSetCurrentPagePosition( d->pages[ d->vectorIndex - 1 ]->pageNumber(), 1.0 );
    else
    {   // go towards the top of current page
        int newValue = QMAX( verticalScrollBar()->value() - height() + 50,
                             verticalScrollBar()->minValue() );
        verticalScrollBar()->setValue( newValue );
    }
}

void PageView::scrollDown()
{
    if( atBottom() && d->vectorIndex < (int)d->pages.count() - 1 )
        // go to the top of previous page
        d->document->slotSetCurrentPagePosition( d->pages[ d->vectorIndex + 1 ]->pageNumber(), 0.0 );
    else
    {    // go towards the bottom of current page
        int newValue = QMIN( verticalScrollBar()->value() + height() - 50,
                             verticalScrollBar()->maxValue() );
        verticalScrollBar()->setValue( newValue );
    }
}

#include "pageview.moc"
