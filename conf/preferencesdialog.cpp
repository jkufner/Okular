/***************************************************************************
 *   Copyright (C) 2004 by Enrico Ros <eros.kde@email.it>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

// reimplementing this
#include "preferencesdialog.h"

#include <klocale.h>

// single config pages
#include "dlggeneral.h"
#include "dlgperformance.h"
#include "dlgaccessibility.h"
#include "dlgpresentation.h"
#include "dlgidentity.h"
#include "dlgdebug.h"

PreferencesDialog::PreferencesDialog( QWidget * parent, KConfigSkeleton * skeleton )
    : KConfigDialog( parent, "preferences", skeleton )
{
    m_general = new DlgGeneral( this );
    m_performance = new DlgPerformance( this );
    m_accessibility = new DlgAccessibility( this );
    m_presentation = new DlgPresentation( this );
    m_identity = new DlgIdentity( this );
#ifdef OKULAR_DEBUG_CONFIGPAGE
    m_debug = new DlgDebug( this );
#endif

    addPage( m_general, i18n("General"), "graphics-viewer-document", i18n("General Options") );
    addPage( m_accessibility, i18n("Accessibility"), "preferences-desktop-accessibility", i18n("Accessibility Reading Aids") );
    addPage( m_performance, i18n("Performance"), "preferences-system-performance", i18n("Performance Tuning") );
    addPage( m_presentation, i18n("Presentation"), "view-presentation",
             i18n("Options for Presentation Mode") );
    addPage( m_identity, i18n("Identity"), "preferences-desktop-personal",
             i18n("Identity Settings") );
#ifdef OKULAR_DEBUG_CONFIGPAGE
    addPage( m_debug, "Debug", "system-run", "Debug options" );
#endif
}
