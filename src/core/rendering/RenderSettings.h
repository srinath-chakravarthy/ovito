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


#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/animation/TimeInterval.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/AnimationSettings.h>
#include "FrameBuffer.h"
#include "SceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * Stores general settings for rendering pictures and movies.
 */
class OVITO_CORE_EXPORT RenderSettings : public RefTarget
{
public:

	/// This enumeration specifies the animation range that should be rendered.
	enum RenderingRangeType {
		CURRENT_FRAME,		///< Render only the current animation.
		ANIMATION_INTERVAL,	///< Render the complete animation interval.
		CUSTOM_INTERVAL,	///< Render time interval defined by the user.
	};
	Q_ENUMS(RenderingRangeType);
	
public:

	/// Constructor.
	/// Creates an instance of the default renderer class which can be accessed via the renderer() method.
	Q_INVOKABLE RenderSettings(DataSet* dataset);

	/// Returns the aspect ratio (height/width) of the rendered image.
	FloatType outputImageAspectRatio() const { return (FloatType)outputImageHeight() / (FloatType)outputImageWidth(); }

	/// Returns the background color of the rendered image.
	Color backgroundColor() const { return backgroundColorController() ? backgroundColorController()->currentColorValue() : Color(0,0,0); }
	/// Sets the background color of the rendered image.
	void setBackgroundColor(const Color& color) { if(backgroundColorController()) backgroundColorController()->setCurrentColorValue(color); }

	/// Returns the output filename of the rendered image.
	const QString& imageFilename() const { return _imageInfo.filename(); }
	/// Sets the output filename of the rendered image.
	void setImageFilename(const QString& filename);

	/// Returns the output image info of the rendered image.
	const ImageInfo& imageInfo() const { return _imageInfo; }
	/// Sets the output image info for the rendered image.
	void setImageInfo(const ImageInfo& imageInfo);

public:

	Q_PROPERTY(QString imageFilename READ imageFilename WRITE setImageFilename);

protected:

	/// Saves the class' contents to the given stream. 
	virtual void saveToStream(ObjectSaveStream& stream) override;
	/// Loads the class' contents from the given stream. 
	virtual void loadFromStream(ObjectLoadStream& stream) override;
	/// Creates a copy of this object. 
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;
	
private:

	/// Contains the output filename and format of the image to be rendered.
	ImageInfo _imageInfo;

	/// The instance of the plugin renderer class. 
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SceneRenderer, renderer, setRenderer);

	/// Controls the background color of the rendered image.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, backgroundColorController, setBackgroundColorController);
	
	/// The width of the output image in pixels.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, outputImageWidth, setOutputImageWidth);

	/// The height of the output image in pixels.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, outputImageHeight, setOutputImageHeight);

	/// Controls whether the alpha channel will be included in the output image.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, generateAlphaChannel, setGenerateAlphaChannel);

	/// Controls whether the rendered image is saved to the output file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, saveToFile, setSaveToFile);

	/// Controls whether already rendered frames are skipped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, skipExistingImages, setSkipExistingImages);

	/// Specifies which part of the animation should be rendered.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(RenderingRangeType, renderingRangeType, setRenderingRangeType);

	/// The first frame to render when rendering range is set to CUSTOM_INTERVAL.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, customRangeStart, setCustomRangeStart);

	/// The last frame to render when rendering range is set to CUSTOM_INTERVAL.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, customRangeEnd, setCustomRangeEnd);

	/// Specifies the number of frames to skip when rendering an animation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Specifies the base number for filename generation when rendering an animation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, fileNumberBase, setFileNumberBase);

private:
    
    friend class RenderSettingsEditor;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::RenderSettings::RenderingRangeType);
Q_DECLARE_TYPEINFO(Ovito::RenderSettings::RenderingRangeType, Q_PRIMITIVE_TYPE);


