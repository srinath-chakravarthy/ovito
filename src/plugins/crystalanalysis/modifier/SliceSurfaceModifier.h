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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/modify/SliceModifier.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurface.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Slice function that operates on surface meshes.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SliceSurfaceFunction : public SliceModifierFunction
{
public:

	/// Constructor.
	Q_INVOKABLE SliceSurfaceFunction(DataSet* dataset) : SliceModifierFunction(dataset) {}

	/// \brief Applies a slice operation to a data object.
	virtual PipelineStatus apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth) override;

	/// \brief Returns whether this slice function can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override {
		return (input.findObject<SurfaceMesh>() != nullptr) || (input.findObject<PartitionMesh>() != nullptr) || (input.findObject<SlipSurface>() != nullptr);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief Slice function that operates on dislocation lines.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SliceDislocationsFunction : public SliceModifierFunction
{
public:

	/// Constructor.
	Q_INVOKABLE SliceDislocationsFunction(DataSet* dataset) : SliceModifierFunction(dataset) {}

	/// \brief Applies a slice operation to a data object.
	virtual PipelineStatus apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth) override;

	/// \brief Returns whether this slice function can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override {
		return (input.findObject<DislocationNetworkObject>() != nullptr);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


