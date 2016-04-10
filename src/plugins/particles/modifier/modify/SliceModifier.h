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

#ifndef __OVITO_SLICE_MODIFIER_H
#define __OVITO_SLICE_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/animation/controller/Controller.h>
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief The slice modifier deletes all particles on one side of a 3d plane.
 */
class OVITO_PARTICLES_EXPORT SliceModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE SliceModifier(DataSet* dataset);

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Lets the modifier render itself into the viewport.
	virtual void render(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay) override;

	/// Computes the bounding box of the visual representation of the modifier.
	virtual Box3 boundingBox(TimePoint time,  ObjectNode* contextNode, ModifierApplication* modApp) override;

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override;

	// Property access functions:

	/// Returns the plane's distance from the origin.
	FloatType distance() const { return _distanceCtrl ? _distanceCtrl->currentFloatValue() : 0.0f; }

	/// Sets the plane's distance from the origin.
	void setDistance(FloatType newDistance) { if(_distanceCtrl) _distanceCtrl->setCurrentFloatValue(newDistance); }

	/// Returns the controller for the plane distance.
	Controller* distanceController() const { return _distanceCtrl; }

	/// Sets the controller for the plane distance.
	void setDistanceController(Controller* ctrl) { _distanceCtrl = ctrl; }

	/// Returns the plane's normal vector.
	Vector3 normal() const { return _normalCtrl ? _normalCtrl->currentVector3Value() : Vector3(0,0,1); }

	/// Sets the plane's distance from the origin.
	void setNormal(const Vector3& newNormal) { if(_normalCtrl) _normalCtrl->setCurrentVector3Value(newNormal); }

	/// Returns the controller for the plane normal.
	Controller* normalController() const { return _normalCtrl; }

	/// Sets the controller for the plane normal.
	void setNormalController(Controller* ctrl) { _normalCtrl = ctrl; }

	/// Returns the slice width.
	FloatType sliceWidth() const { return _widthCtrl ? _widthCtrl->currentFloatValue() : 0.0f; }

	/// Sets the slice width.
	void setSliceWidth(FloatType newWidth) { if(_widthCtrl) _widthCtrl->setCurrentFloatValue(newWidth); }

	/// Returns the controller for the slice width.
	Controller* sliceWidthController() const { return _widthCtrl; }

	/// Sets the controller for the slice width.
	void setSliceWidthController(Controller* ctrl) { _widthCtrl = ctrl; }

	/// Returns whether the plane's orientation should be flipped.
	bool inverse() const { return _inverse; }

	/// Sets whether the plane's orientation should be flipped.
	void setInverse(bool inverse) { _inverse = inverse; }

	/// Returns whether the atoms are only selected instead of deleted.
	bool createSelection() const { return _createSelection; }

	/// Sets whether the atoms are only selected instead of deleted.
	void setCreateSelection(bool select) { _createSelection = select; }

	/// Returns whether the modifier is only applied to the currently selected atoms.
	bool applyToSelection() const { return _applyToSelection; }

	/// Sets whether the modifier should only be applied to the currently selected atoms.
	void setApplyToSelection(bool flag) { _applyToSelection = flag; }

	/// Returns the slicing plane.
	Plane3 slicingPlane(TimePoint time, TimeInterval& validityInterval);

protected:

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// \brief Renders the modifier's visual representation and computes its bounding box.
	Box3 renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer);

	/// Renders the plane in the viewport.
	Box3 renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& box, const ColorA& color) const;

	/// Computes the intersection lines of a plane and a quad.
	void planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const;

	/// This controller stores the normal of the slicing plane.
	ReferenceField<Controller> _normalCtrl;

	/// This controller stores the distance of the slicing plane from the origin.
	ReferenceField<Controller> _distanceCtrl;

	/// Controls the slice width.
	ReferenceField<Controller> _widthCtrl;

	/// Controls whether the atoms should only be selected instead of deleted.
	PropertyField<bool> _createSelection;

	/// Controls whether the selection/plane orientation should be inverted.
	PropertyField<bool> _inverse;

	/// Controls whether the modifier should only be applied to the currently selected atoms.
	PropertyField<bool> _applyToSelection;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Slice");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_REFERENCE_FIELD(_normalCtrl);
	DECLARE_REFERENCE_FIELD(_distanceCtrl);
	DECLARE_REFERENCE_FIELD(_widthCtrl);
	DECLARE_PROPERTY_FIELD(_createSelection);
	DECLARE_PROPERTY_FIELD(_inverse);
	DECLARE_PROPERTY_FIELD(_applyToSelection);
};

/**
 * \brief Abstract base class for slice functions that operate on different kinds of data.
 */
class OVITO_PARTICLES_EXPORT SliceModifierFunction : public RefTarget
{
protected:

	/// Abstract class constructor.
	SliceModifierFunction(DataSet* dataset) : RefTarget(dataset) {}

public:

	/// \brief Applies a slice operation to a data object.
	virtual PipelineStatus apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth) = 0;

	/// \brief Returns whether this slice function can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) = 0;

private:

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief Slice function that operates on particles.
 */
class OVITO_PARTICLES_EXPORT SliceParticlesFunction : public SliceModifierFunction
{
public:

	/// Constructor.
	Q_INVOKABLE SliceParticlesFunction(DataSet* dataset) : SliceModifierFunction(dataset) {}

	/// \brief Applies a slice operation to a data object.
	virtual PipelineStatus apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth) override;

	/// \brief Returns whether this slice function can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override {
		return (input.findObject<ParticlePropertyObject>() != nullptr);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_SLICE_MODIFIER_H
