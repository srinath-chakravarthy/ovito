///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/reference/RefTarget.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief An object that stores the current VR settings.
 */
class VRSettingsObject : public RefTarget
{
public:

	/// \brief Default constructor.
	Q_INVOKABLE VRSettingsObject(DataSet* dataset) : RefTarget(dataset),
		_supersamplingEnabled(true), _scaleFactor(1e-1), _showFloor(false), _flyingMode(false), 
		_translation(Vector3::Zero()),
		_rotationZ(0),
		_modelCenter(Vector3::Zero()),
		_viewerTM(AffineTransformation::Identity()),
		_movementSpeed(4) {
		INIT_PROPERTY_FIELD(supersamplingEnabled);
		INIT_PROPERTY_FIELD(scaleFactor);
		INIT_PROPERTY_FIELD(translation);
		INIT_PROPERTY_FIELD(rotationZ);
		INIT_PROPERTY_FIELD(modelCenter);
		INIT_PROPERTY_FIELD(showFloor);
		INIT_PROPERTY_FIELD(flyingMode);
		INIT_PROPERTY_FIELD(viewerTM);
		INIT_PROPERTY_FIELD(movementSpeed);
	}

	/// Adjusts the transformation to bring the model into the center of the playing area.
	void recenter();

	/// Computes the apparent model size in meters.
	Vector3 apparentModelSize();

private:

	/// Enables supersampling.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, supersamplingEnabled, setSupersamplingEnabled);

	/// The scaling applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, scaleFactor, setScaleFactor);

	/// The translation applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, translation, setTranslation);

	/// The rotation angle around vertical axis applied to the model.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, rotationZ, setRotationZ);

	/// The center point of the model, around which it is being rotated.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, modelCenter, setModelCenter);

	/// Enables the display of the floor rectangle.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showFloor, setShowFloor);

	/// Activates the flying mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, flyingMode, setFlyingMode);

	/// Current flying position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, viewerTM, setViewerTM);

	/// The speed of motion when navigating.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, movementSpeed, setMovementSpeed);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
