///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski, Tobias Brink
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

#ifndef __OVITO_PARTICLES_SCRIPT_BINDING_H
#define __OVITO_PARTICLES_SCRIPT_BINDING_H

#include <plugins/particles/Particles.h>
#include <plugins/scripting/engine/ScriptBinding.h>
#include <plugins/particles/data/ParticlePropertyObject.h>

namespace Particles {

using namespace Ovito;
using namespace Scripting;

/**
 * \brief Provides script bindings for the classes in the Particles plugin.
 */
class ParticlesBinding : public ScriptBinding
{
public:

	/// \brief Default constructor.
	Q_INVOKABLE ParticlesBinding() {}

	/// \brief Sets up the global object of the script engine.
	virtual void setupBinding(ScriptEngine& engine) override;

	/// Creates a QScriptValue from a ParticlePropertyReference.
	static QScriptValue fromParticlePropertyReference(QScriptEngine* engine, const ParticlePropertyReference& pref);

	/// Converts a QScriptValue to a ParticlePropertyReference.
	static void toParticlePropertyReference(const QScriptValue& obj, ParticlePropertyReference& pref);

private:

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif
