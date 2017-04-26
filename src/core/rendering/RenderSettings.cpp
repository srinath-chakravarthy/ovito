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

#include <core/Core.h>
#include <core/rendering/SceneRenderer.h>
#include <core/viewport/Viewport.h>
#include <core/plugins/PluginManager.h>
#include "RenderSettings.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(RenderSettings, RefTarget);
DEFINE_FLAGS_REFERENCE_FIELD(RenderSettings, renderer, "Renderer", SceneRenderer, PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(RenderSettings, backgroundColorController, "BackgroundColor", Controller, PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(RenderSettings, outputImageWidth, "OutputImageWidth");
DEFINE_PROPERTY_FIELD(RenderSettings, outputImageHeight, "OutputImageHeight");
DEFINE_PROPERTY_FIELD(RenderSettings, generateAlphaChannel, "GenerateAlphaChannel");
DEFINE_PROPERTY_FIELD(RenderSettings, saveToFile, "SaveToFile");
DEFINE_PROPERTY_FIELD(RenderSettings, skipExistingImages, "SkipExistingImages");
DEFINE_PROPERTY_FIELD(RenderSettings, renderingRangeType, "RenderingRangeType");
DEFINE_PROPERTY_FIELD(RenderSettings, customRangeStart, "CustomRangeStart");
DEFINE_PROPERTY_FIELD(RenderSettings, customRangeEnd, "CustomRangeEnd");
DEFINE_PROPERTY_FIELD(RenderSettings, everyNthFrame, "EveryNthFrame");
DEFINE_PROPERTY_FIELD(RenderSettings, fileNumberBase, "FileNumberBase");
SET_PROPERTY_FIELD_LABEL(RenderSettings, renderer, "Renderer");
SET_PROPERTY_FIELD_LABEL(RenderSettings, backgroundColorController, "Background color");
SET_PROPERTY_FIELD_LABEL(RenderSettings, outputImageWidth, "Width");
SET_PROPERTY_FIELD_LABEL(RenderSettings, outputImageHeight, "Height");
SET_PROPERTY_FIELD_LABEL(RenderSettings, generateAlphaChannel, "Transparent background");
SET_PROPERTY_FIELD_LABEL(RenderSettings, saveToFile, "Save to file");
SET_PROPERTY_FIELD_LABEL(RenderSettings, skipExistingImages, "Skip existing animation images");
SET_PROPERTY_FIELD_LABEL(RenderSettings, renderingRangeType, "Rendering range");
SET_PROPERTY_FIELD_LABEL(RenderSettings, customRangeStart, "Range start");
SET_PROPERTY_FIELD_LABEL(RenderSettings, customRangeEnd, "Range end");
SET_PROPERTY_FIELD_LABEL(RenderSettings, everyNthFrame, "Every Nth frame");
SET_PROPERTY_FIELD_LABEL(RenderSettings, fileNumberBase, "File number base");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageWidth, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, outputImageHeight, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RenderSettings, everyNthFrame, IntegerParameterUnit, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
RenderSettings::RenderSettings(DataSet* dataset) : RefTarget(dataset),
	_outputImageWidth(640), _outputImageHeight(480), _generateAlphaChannel(false),
	_saveToFile(false), _skipExistingImages(false), _renderingRangeType(CURRENT_FRAME),
	_customRangeStart(0), _customRangeEnd(100), _everyNthFrame(1), _fileNumberBase(0)
{
	INIT_PROPERTY_FIELD(renderer);
	INIT_PROPERTY_FIELD(backgroundColorController);
	INIT_PROPERTY_FIELD(outputImageWidth);
	INIT_PROPERTY_FIELD(outputImageHeight);
	INIT_PROPERTY_FIELD(generateAlphaChannel);
	INIT_PROPERTY_FIELD(saveToFile);
	INIT_PROPERTY_FIELD(skipExistingImages);
	INIT_PROPERTY_FIELD(renderingRangeType);
	INIT_PROPERTY_FIELD(customRangeStart);
	INIT_PROPERTY_FIELD(customRangeEnd);
	INIT_PROPERTY_FIELD(everyNthFrame);
	INIT_PROPERTY_FIELD(fileNumberBase);

	// Setup default background color.
	setBackgroundColorController(ControllerManager::createColorController(dataset));
	setBackgroundColor(Color(1,1,1));

	// Create an instance of the default renderer class.
	Plugin* glrendererPlugin = PluginManager::instance().plugin("OpenGLRenderer");
	OvitoObjectType* rendererClass = nullptr;
	if(glrendererPlugin)
		rendererClass = glrendererPlugin->findClass("StandardSceneRenderer");
	if(rendererClass == nullptr) {
		QVector<OvitoObjectType*> classList = PluginManager::instance().listClasses(SceneRenderer::OOType);
		if(classList.isEmpty() == false) rendererClass = classList.front();
	}
	if(rendererClass)
		setRenderer(static_object_cast<SceneRenderer>(rendererClass->createInstance(dataset)));
}

/******************************************************************************
* Sets the output filename of the rendered image. 
******************************************************************************/
void RenderSettings::setImageFilename(const QString& filename)
{
	if(filename == imageFilename()) return;
	_imageInfo.setFilename(filename);
	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Sets the output image info of the rendered image.
******************************************************************************/
void RenderSettings::setImageInfo(const ImageInfo& imageInfo)
{
	if(imageInfo == _imageInfo) return;
	_imageInfo = imageInfo;
	notifyDependents(ReferenceEvent::TargetChanged);
}

#define RENDER_SETTINGS_FILE_FORMAT_VERSION		1

/******************************************************************************
* Saves the class' contents to the given stream. 
******************************************************************************/
void RenderSettings::saveToStream(ObjectSaveStream& stream)
{
	RefTarget::saveToStream(stream);

	stream.beginChunk(RENDER_SETTINGS_FILE_FORMAT_VERSION);
	stream << _imageInfo;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream. 
******************************************************************************/
void RenderSettings::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);

	int fileVersion = stream.expectChunkRange(0, RENDER_SETTINGS_FILE_FORMAT_VERSION);
	if(fileVersion == 0) {
		bool generateAlphaChannel;
		RenderingRangeType renderingRange;
		stream >> renderingRange;
		stream >> _imageInfo;
		stream >> generateAlphaChannel;
		_generateAlphaChannel = generateAlphaChannel;
		_renderingRangeType = renderingRange;
		_outputImageWidth = _imageInfo.imageWidth();
		_outputImageHeight = _imageInfo.imageHeight();
	}
	else {
		stream >> _imageInfo;
	}
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object. 
******************************************************************************/
OORef<RefTarget> RenderSettings::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<RenderSettings> clone = static_object_cast<RenderSettings>(RefTarget::clone(deepCopy, cloneHelper));
	
	/// Copy data values.
	clone->_imageInfo = this->_imageInfo;

	return clone;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
