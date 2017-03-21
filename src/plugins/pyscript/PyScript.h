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

#pragma once


#include <core/Core.h>

// Qt defines the 'slots' and 'signals' keyword macros. They are in conflict with identifiers used in the Python headers.
#ifdef slots
	#undef slots
#endif
#ifdef signals
	#undef signals
#endif

// Include pybind11, which is located in our 3rd party source directory.
#include <3rdparty/pybind11/pybind11.h>
#include <3rdparty/pybind11/eval.h>
#include <3rdparty/pybind11/operators.h>
#include <3rdparty/pybind11/stl_bind.h>
#include <3rdparty/pybind11/stl.h>
#include <3rdparty/pybind11/numpy.h>

#ifdef PyScript_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_PYSCRIPT_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_PYSCRIPT_EXPORT Q_DECL_IMPORT
#endif
