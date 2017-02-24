///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

/**
 * \file
 * \brief This file includes third-party library headers required by OVITO's GUI.
 */

#pragma once


#include <core/Core.h>

/******************************************************************************
* QT Library
******************************************************************************/
#include <QApplication>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QResource>
#include <QtWidgets>
#include <QtDebug>
#include <QtGui>
#include <QCommandLineParser>

#ifdef Gui_EXPORTS		// This is defined by CMake when building the Gui library.
#  define OVITO_GUI_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_GUI_EXPORT Q_DECL_IMPORT
#endif

/*! \namespace Ovito::Gui
    \brief This namespace contains the graphical user interface classes.
*/
/*! \namespace Ovito::Gui::Widgets
    \brief This namespace contains the widget classes that can be used in the graphical user interface.
*/
/*! \namespace Ovito::Gui::Params
    \brief This namespace contains GUI classes for parameter editing.
*/
/*! \namespace Ovito::Gui::ViewportInput
    \brief This namespace contains classes for interaction with the viewports.
*/
/*! \namespace Ovito::Gui::Dialogs
    \brief This namespace contains common dialog box classes.
*/

/******************************************************************************
* Forward declaration of classes.
******************************************************************************/
#include "ForwardDecl.h"



