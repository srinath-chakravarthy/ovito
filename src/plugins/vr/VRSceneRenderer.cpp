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

#include <gui/GUI.h>
#include <core/dataset/DataSet.h>
#include <core/rendering/RenderSettings.h>
#include "VRSceneRenderer.h"

namespace VRPlugin {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(VRSceneRenderer, OpenGLSceneRenderer);

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void VRSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	OpenGLSceneRenderer::beginFrame(time, params, vp);

	// Set viewport background color.
	setClearColor(ColorA(renderSettings()->backgroundColor()));
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize VRSceneRenderer::outputSize() const
{
#if 0
	return viewport()->windowSize();
#endif
	return OpenGLSceneRenderer::outputSize();
}

/******************************************************************************
* Returns the device pixel ratio of the output device we are rendering to.
******************************************************************************/
qreal VRSceneRenderer::devicePixelRatio() const
{
#if 0
	if(viewport()) {
		if(QWidget* vpWidget = ViewportsPanel::viewportWidget(viewport()))
			return vpWidget->devicePixelRatio();
	}
#endif
	return OpenGLSceneRenderer::devicePixelRatio();
}

}	// End of namespace
