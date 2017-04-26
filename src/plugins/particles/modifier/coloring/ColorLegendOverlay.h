///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <core/viewport/overlay/ViewportOverlay.h>
#include "ColorCodingModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

/**
 * \brief A viewport overlay that displays the color legend of a ColorCodingModifier.
 */
class OVITO_PARTICLES_EXPORT ColorLegendOverlay : public ViewportOverlay
{
public:

	/// \brief Constructor.
	Q_INVOKABLE ColorLegendOverlay(DataSet* dataset);

	/// \brief This method asks the overlay to paint its contents over the given viewport.
	virtual void render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings) override;

	/// Moves the position of the overlay in the viewport by the given amount,
	/// which is specified as a fraction of the viewport render size.
	virtual void moveOverlayInViewport(const Vector2& delta) override {
		_offsetX = _offsetX + delta.x();
		_offsetY = _offsetY + delta.y();
	}

public:

	Q_PROPERTY(Ovito::Particles::ColorCodingModifier* modifier READ modifier WRITE setModifier);

private:

	/// The corner of the viewport where the color legend is displayed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, alignment, setAlignment);

	/// The orientation (horizontal/vertical) of the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, orientation, setOrientation);

	/// Controls the overall size of the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, legendSize, setLegendSize);

	/// Controls the aspect ration of the color bar.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, aspectRatio, setAspectRatio);

	/// Controls the horizontal offset of legend position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetX, setOffsetX)

	/// Controls the vertical offset of legend position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetY, setOffsetY);

	/// Controls the label font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QFont, font, setFont);

	/// Controls the label font size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, fontSize, setFontSize);

	/// The title label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, title, setTitle);

	/// User-defined text for the first numeric label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, label1, setLabel1);

	/// User-defined text for the second numeric label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, label2, setLabel2);

	/// The ColorCodingModifier for which to display the legend.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(ColorCodingModifier, modifier, setModifier);

	/// Controls the formatting of the value labels in the color legend.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, valueFormatString, setValueFormatString);

	/// Controls the text color.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, textColor, setTextColor);

	/// The text outline color.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, outlineColor, setOutlineColor);

	/// Controls the outlining of the font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outlineEnabled, setOutlineEnabled)

	Q_CLASSINFO("DisplayName", "Color legend");

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


