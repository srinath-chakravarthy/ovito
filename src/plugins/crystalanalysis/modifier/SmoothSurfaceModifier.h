///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Smoothes and fairs the defect surface mesh.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SmoothSurfaceModifier : public Modifier
{
public:

	/// Constructor.
	Q_INVOKABLE SmoothSurfaceModifier(DataSet* dataset);

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override;

	/// This modifies the input object.
	virtual PipelineStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

private:

	/// Controls the amount of smoothing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, smoothingLevel, setSmoothingLevel);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Smooth surface");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


