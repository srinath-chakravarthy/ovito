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

#include <core/Core.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/DataSet.h>
#include <core/scene/SceneRoot.h>
#include <core/viewport/ViewportSettings.h>
#include "VRSettingsObject.h"

namespace VRPlugin {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(VRSettingsObject, RefTarget);
DEFINE_PROPERTY_FIELD(VRSettingsObject, supersamplingEnabled, "SupersamplingEnabled");
DEFINE_FLAGS_PROPERTY_FIELD(VRSettingsObject, scaleFactor, "ScaleFactor", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(VRSettingsObject, showFloor, "ShowFloor");
DEFINE_FLAGS_PROPERTY_FIELD(VRSettingsObject, flyingMode, "FlyingMode", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(VRSettingsObject, viewerTM, "ViewerTM");
DEFINE_PROPERTY_FIELD(VRSettingsObject, translation, "Translation");
DEFINE_PROPERTY_FIELD(VRSettingsObject, rotationZ, "RotationZ");
DEFINE_PROPERTY_FIELD(VRSettingsObject, modelCenter, "ModelCenter");
DEFINE_PROPERTY_FIELD(VRSettingsObject, movementSpeed, "MovementSpeed");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, supersamplingEnabled, "Supersampling");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, scaleFactor, "Scale factor");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, translation, "Position");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, rotationZ, "Rotation angle");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, showFloor, "Show floor rectangle");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, flyingMode, "Fly mode");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, viewerTM, "Viewer transformation");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, modelCenter, "Center of rotation");
SET_PROPERTY_FIELD_LABEL(VRSettingsObject, movementSpeed, "Speed");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VRSettingsObject, scaleFactor, PercentParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS(VRSettingsObject, rotationZ, AngleParameterUnit);
SET_PROPERTY_FIELD_UNITS(VRSettingsObject, modelCenter, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VRSettingsObject, movementSpeed, FloatParameterUnit, 0);

/******************************************************************************
* Adjusts the transformation to bring the model into the center of the 
* playing area.
******************************************************************************/
void VRSettingsObject::recenter()
{
    // Reset model position to center of scene bounding box.
    const Box3& bbox = dataset()->sceneRoot()->worldBoundingBox(dataset()->animationSettings()->time());
    if(!bbox.isEmpty()) {
        setModelCenter(bbox.center() - Point3::Origin());
    }
    setRotationZ(0);
    if(flyingMode() == false) {
        FloatType height = bbox.size(ViewportSettings::getSettings().upDirection()) * scaleFactor() / FloatType(1.9);
        setTranslation(Vector3(0, 0, height));
        setViewerTM(AffineTransformation::Identity());
    }
    else {
        FloatType offset = bbox.size().length() * scaleFactor() / 2;
        setTranslation(Vector3(0, 0, 0));
        setViewerTM(AffineTransformation::translation(
            (ViewportSettings::getSettings().coordinateSystemOrientation() *
            AffineTransformation::rotationX(FLOATTYPE_PI/2)).inverse() *
            Vector3(0, -offset, 0)));
    }
}

/******************************************************************************
* Computes the apparent model size in meters.
******************************************************************************/
Vector3 VRSettingsObject::apparentModelSize()
{
    const Box3& bbox = dataset()->sceneRoot()->worldBoundingBox(dataset()->animationSettings()->time());
    if(!bbox.isEmpty())
        return bbox.size() * scaleFactor();
    else
        return Vector3::Zero();
}


};
