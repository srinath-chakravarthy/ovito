///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
 * \brief This file includes STL and third-party library headers required by OVITO. It is included by all .cpp files belonging to OVITO's codebase.
 */

#pragma once


/******************************************************************************
* Standard Template Library (STL)
******************************************************************************/
#include <iostream>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <stack>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <utility>
#include <random>
#include <memory>
#include <mutex>
#include <thread>
#include <clocale>
#include <atomic>
#include <tuple>
#include <numeric>

/******************************************************************************
* QT Library
******************************************************************************/
#include <QCoreApplication>
#include <QException>
#include <QStringList>
#include <QSettings>
#include <QUrl>
#include <QPointer>
#include <QFileInfo>
#include <QResource>
#include <QDir>
#include <QtDebug>
#include <QtGlobal>
#include <QMetaClassInfo>
#include <QColor>
#include <QGenericMatrix>
#include <QMatrix4x4>
#include <QDateTime>
#include <QThread>
#include <QImage>
#include <QFont>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QCommandLineParser>
#include <QGuiApplication>

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
#  error "OVITO requires Qt 5.4 or newer."
#endif

/******************************************************************************
* Boost Library
******************************************************************************/
#include <boost/dynamic_bitset.hpp>


#ifdef Core_EXPORTS		// This is defined by CMake when building the Core library.
#  define OVITO_CORE_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_CORE_EXPORT Q_DECL_IMPORT
#endif

/*! \namespace Ovito
    \brief The root namespace of OVITO.
*/
/*! \namespace Ovito::Util
    \brief This namespace contains general utility classes and typedefs used throughout OVITO's codebase.
*/
/*! \namespace Ovito::IO
    \brief This namespace contains I/O-related utility classes.
*/
/*! \namespace Ovito::Concurrency
    \brief This namespace contains class related to multi-threading, parallelism, and asynchronous tasks.
*/
/*! \namespace Ovito::Mesh
    \brief This namespace contains classes for working with triangular and polyhedral meshes.
*/
/*! \namespace Ovito::Math
    \brief This namespace contains classes related to linear algebra and geometry (vectors, transformation matrices, etc).
*/
/*! \namespace Ovito::Rendering
    \brief This namespace contains classes related to scene rendering.
*/
/*! \namespace Ovito::View
    \brief This namespace contains classes related to 3d viewports.
*/
/*! \namespace Ovito::DataIO
    \brief This namespace contains the framework for data import and export.
*/
/*! \namespace Ovito::Anim
    \brief This namespace contains classes related to object and parameter animation.
*/
/*! \namespace Ovito::PluginSystem
    \brief This namespace contains classes related to OVITO's plugin-based extension system.
*/
/*! \namespace Ovito::ObjectSystem
    \brief This namespace contains basic classes of OVITO's object system.
*/
/*! \namespace Ovito::Units
    \brief This namespace contains classes related to parameter units.
*/
/*! \namespace Ovito::Undo
    \brief This namespace contains the implementation of OVITO's undo framework.
*/
/*! \namespace Ovito::Scene
    \brief This namespace contains the scene graph and modification pipeline framework.
*/

/******************************************************************************
* Forward declaration of classes.
******************************************************************************/
#include "ForwardDecl.h"

/******************************************************************************
* Our own basic headers
******************************************************************************/
#include <core/utilities/Debugging.h>
#include <core/utilities/FloatType.h>
#include <core/utilities/Exception.h>
#include <core/utilities/linalg/LinAlg.h>
#include <core/utilities/Color.h>
#include <core/object/OvitoObject.h>


