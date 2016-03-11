///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#ifndef __OVITO_VOROTOP_PLUGIN_H
#define __OVITO_VOROTOP_PLUGIN_H

#include <plugins/particles/Particles.h>

#ifdef VoroTop_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_VOROTOP_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_VOROTOP_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito {
	namespace Plugins {
		namespace VoroTop {
			using namespace Ovito::Particles;
		}
	}
}

#endif
