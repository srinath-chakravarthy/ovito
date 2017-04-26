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

#pragma once


#include <gui/GUI.h>
#include <plugins/particles/Particles.h>

#ifdef ParticlesGui_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_PARTICLES_GUI_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_PARTICLES_GUI_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito {
	namespace Particles {
		OVITO_BEGIN_INLINE_NAMESPACE(Util)
			class ParticlePropertyComboBox;
		OVITO_END_INLINE_NAMESPACE
	}
}
