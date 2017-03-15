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
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <core/scene/pipeline/Modifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Smoothes the dislocation lines.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SmoothDislocationsModifier : public Modifier
{
public:

	/// Constructor.
	Q_INVOKABLE SmoothDislocationsModifier(DataSet* dataset);

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override;

	/// This modifies the input object.
	virtual PipelineStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// Smoothes the given dislocation lines.
	void smoothDislocationLines(DislocationNetworkObject* dislocationsObj);

protected:

	/// Removes some of the sampling points from a dislocation line.
	void coarsenDislocationLine(FloatType linePointInterval, const std::deque<Point3>& input, const std::deque<int>& coreSize, std::deque<Point3>& output, std::deque<int>& outputCoreSize, bool isClosedLoop, bool isInfiniteLine);

	/// Smoothes the sampling points of a dislocation line.
	void smoothDislocationLine(int smoothingLevel, std::deque<Point3>& line, bool isLoop);

private:

	/// Stores whether smoothing is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothingEnabled, setSmoothingEnabled);

	/// Controls the degree of smoothing.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, smoothingLevel, setSmoothingLevel);

	/// Stores whether coarsening is enabled.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, coarseningEnabled, setCoarseningEnabled);

	/// Controls the coarsening of dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, linePointInterval, setLinePointInterval);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Smooth dislocations");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


