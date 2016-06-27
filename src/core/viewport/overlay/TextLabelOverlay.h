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

#ifndef __OVITO_TEXT_LABEL_OVERLAY_H
#define __OVITO_TEXT_LABEL_OVERLAY_H

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

	/// Returns the corner of the viewport where the label is shown.
	int alignment() const { return _alignment; }

	/// Sets the corner of the viewport where the label is shown.
	void setAlignment(int align) { _alignment = align; }

	/// Returns the horizontal offset of the label's position.
	FloatType offsetX() const { return _offsetX; }

	/// Sets the horizontal offset of the label's position.
	void setOffsetX(FloatType offset) { _offsetX = offset; }

	/// Returns the vertical offset of the label's position.
	FloatType offsetY() const { return _offsetY; }

	/// Sets the vertical offset of the label's position.
	void setOffsetY(FloatType offset) { _offsetY = offset; }

	/// Returns the text label font.
	const QFont& font() const { return _font; }

	/// Sets the text label font.
	void setFont(const QFont& font) { _font = font; }

	/// Returns the text label font size.
	FloatType fontSize() const { return _fontSize; }

	/// Sets the text label font size.
	void setFontSize(FloatType fontSize) { _fontSize = fontSize; }

	/// Returns the user-defined text string.
	const QString& labelText() const { return _labelText; }

	/// Sets the user-defined text string.
	void setLabelText(const QString& text) { _labelText = text; }

	/// Returns the display color of the label.
	const Color& textColor() const { return _textColor; }

	/// Sets the display color of the label.
	void setTextColor(const Color& c) { _textColor = c; }

	/// Returns the outline color of the font.
	const Color& outlineColor() const { return _outlineColor; }

	/// Sets the outline color of the font.
	void setOutlineColor(const Color& c) { _outlineColor = c; }

	/// Returns whether a text outline is drawn.
	bool outlineEnabled() const { return _outlineEnabled; }

	/// Sets whether a text outline is drawn.
	void setOutlineEnabled(bool enable) { _outlineEnabled = enable; }

	/// Returns the ObjectNode providing global attributes that can be reference in the text.
	ObjectNode* sourceNode() const { return _sourceNode; }

	/// Sets the ObjectNode providing global attributes that can be reference in the text.
	void setSourceNode(ObjectNode* node) { _sourceNode = node; }

public:

	Q_PROPERTY(Ovito::ObjectNode* sourceNode READ sourceNode WRITE setSourceNode);

private:

	/// The corner of the viewport where the label is shown in.
	PropertyField<int> _alignment;

	/// Controls the horizontal offset of label position.
	PropertyField<FloatType> _offsetX;

	/// Controls the vertical offset of label position.
	PropertyField<FloatType> _offsetY;

	/// Controls the label font.
	PropertyField<QFont> _font;

	/// Controls the label font size.
	PropertyField<FloatType> _fontSize;

	/// The label's text.
	PropertyField<QString> _labelText;

	/// The display color of the label.
	PropertyField<Color, QColor> _textColor;

	/// The text outline color.
	PropertyField<Color, QColor> _outlineColor;

	/// Controls the outlining of the font.
	PropertyField<bool> _outlineEnabled;

	/// The ObjectNode providing global attributes that can be reference in the text.
	ReferenceField<ObjectNode> _sourceNode;

	DECLARE_PROPERTY_FIELD(_alignment);
	DECLARE_PROPERTY_FIELD(_font);
	DECLARE_PROPERTY_FIELD(_fontSize);
	DECLARE_PROPERTY_FIELD(_offsetX);
	DECLARE_PROPERTY_FIELD(_offsetY);
	DECLARE_PROPERTY_FIELD(_labelText);
	DECLARE_PROPERTY_FIELD(_textColor);
	DECLARE_PROPERTY_FIELD(_outlineColor);
	DECLARE_PROPERTY_FIELD(_outlineEnabled);
	DECLARE_REFERENCE_FIELD(_sourceNode);

	Q_CLASSINFO("DisplayName", "Text label");

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_TEXT_LABEL_OVERLAY_H
