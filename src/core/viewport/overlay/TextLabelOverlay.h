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


#include <core/Core.h>
#include <core/scene/ObjectNode.h>
#include "ViewportOverlay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A viewport overlay that displays a user-defined text label.
 */
class OVITO_CORE_EXPORT TextLabelOverlay : public ViewportOverlay
{
public:

	/// \brief Constructor.
	Q_INVOKABLE TextLabelOverlay(DataSet* dataset);

	/// \brief This method asks the overlay to paint its contents over the given viewport.
	virtual void render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings) override;

	/// Moves the position of the overlay in the viewport by the given amount,
	/// which is specified as a fraction of the viewport render size.
	virtual void moveOverlayInViewport(const Vector2& delta) override {
		setOffsetX(offsetX() + delta.x());
		setOffsetY(offsetY() + delta.y());
	}

public:

	Q_PROPERTY(Ovito::ObjectNode* sourceNode READ sourceNode WRITE setSourceNode);

private:

	/// The corner of the viewport where the label is shown in.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, alignment, setAlignment);

	/// Controls the horizontal offset of label position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetX, setOffsetX);

	/// Controls the vertical offset of label position.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, offsetY, setOffsetY);

	/// Controls the label font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QFont, font, setFont);

	/// Controls the label font size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, fontSize, setFontSize);

	/// The label's text.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, labelText, setLabelText);

	/// The display color of the label.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, textColor, setTextColor);

	/// The text outline color.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, outlineColor, setOutlineColor);

	/// Controls the outlining of the font.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outlineEnabled, setOutlineEnabled);

	/// The ObjectNode providing global attributes that can be reference in the text.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(ObjectNode, sourceNode, setSourceNode);

	Q_CLASSINFO("DisplayName", "Text label");

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


