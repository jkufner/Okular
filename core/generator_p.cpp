/***************************************************************************
 *   Copyright (C) 2007  Tobias Koenig <tokoe@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "generator_p.h"

#include "generator.h"

using namespace Okular;

PixmapGenerationThread::PixmapGenerationThread( Generator *generator )
    : mGenerator( generator ), mRequest( 0 )
{
}

void PixmapGenerationThread::startGeneration( PixmapRequest *request )
{
    mRequest = request;

    start( QThread::InheritPriority );
}

void PixmapGenerationThread::endGeneration()
{
    mRequest = 0;
}

PixmapRequest *PixmapGenerationThread::request() const
{
    return mRequest;
}

QImage PixmapGenerationThread::image() const
{
    return mImage;
}

void PixmapGenerationThread::run()
{
    mImage = QImage();

    if ( mRequest )
        mImage = mGenerator->image( mRequest );
}


TextPageGenerationThread::TextPageGenerationThread( Generator *generator )
    : mGenerator( generator ), mPage( 0 )
{
}

void TextPageGenerationThread::startGeneration( Page *page )
{
    mPage = page;

    start( QThread::InheritPriority );
}

void TextPageGenerationThread::endGeneration()
{
    mPage = 0;
}

Page *TextPageGenerationThread::page() const
{
    return mPage;
}

TextPage* TextPageGenerationThread::textPage() const
{
    return mTextPage;
}

void TextPageGenerationThread::run()
{
    mTextPage = 0;

    if ( mPage )
        mTextPage = mGenerator->textPage( mPage );
}

#include "generator_p.moc"