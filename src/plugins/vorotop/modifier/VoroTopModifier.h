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

#ifndef __OVITO_VORONOI_TOPOLOGY_ANALYSIS_MODIFIER_H
#define __OVITO_VORONOI_TOPOLOGY_ANALYSIS_MODIFIER_H

#include <plugins/vorotop/VoroTopPlugin.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace voro {
	class voronoicell_neighbor;	// Defined by Voro++
}

namespace Ovito { namespace Plugins { namespace VoroTop {

/**
 * \brief This analysis modifier performs the Voronoi topology analysis developed by Emanuel A. Lazar.
 */
class OVITO_VOROTOP_EXPORT VoroTopModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE VoroTopModifier(DataSet* dataset);

	/// Returns whether the modifier takes into account particle radii.
	bool useRadii() const { return _useRadii; }

	/// Sets whether the modifier takes into account particle radii.
	void setUseRadii(bool useRadii) { _useRadii = useRadii; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

private:

	/// Compute engine that performs the actual analysis in a background thread.
	class VoroTopAnalysisEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		VoroTopAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* selection,
							std::vector<FloatType>&& radii, const SimulationCell& simCell) :
			StructureIdentificationEngine(validityInterval, positions, simCell, selection),
			_radii(std::move(radii)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Processes a single Voronoi cell.
		void processCell(voro::voronoicell_neighbor& vcell, size_t particleIndex, QMutex* mutex);

	private:

		std::vector<FloatType> _radii;
	};

	/// Controls whether the weighted Voronoi tessellation is computed, which takes into account particle radii.
	PropertyField<bool> _useRadii;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "VoroTop analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_useRadii);
};

/**
 * A properties editor for the VoroTopModifier class.
 */
class VoroTopModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE VoroTopModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_VORONOI_TOPOLOGY_ANALYSIS_MODIFIER_H
