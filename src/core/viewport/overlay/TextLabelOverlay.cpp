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

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include <core/scene/SelectionSet.h>
#include "TextLabelOverlay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(TextLabelOverlay, ViewportOverlay);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, alignment, "Alignment", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, font, "Font", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, fontSize, "FontSize", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, offsetX, "OffsetX", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, offsetY, "OffsetY", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, labelText, "LabelText");
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, textColor, "TextColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, outlineColor, "OutlineColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, outlineEnabled, "OutlineEnabled", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(TextLabelOverlay, sourceNode, "SourceNode", ObjectNode, PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, alignment, "Position");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, font, "Font");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, textColor, "Text color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, outlineColor, "Outline color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, outlineEnabled, "Enable outline");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, sourceNode, "Attributes source");
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TextLabelOverlay, fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
TextLabelOverlay::TextLabelOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_alignment(Qt::AlignLeft | Qt::AlignTop),
		_offsetX(0), _offsetY(0),
		_fontSize(0.02),
		_labelText("Text label"),
		_textColor(0,0,0.5),
		_outlineColor(1,1,1),
		_outlineEnabled(false)
{
	INIT_PROPERTY_FIELD(alignment);
	INIT_PROPERTY_FIELD(offsetX);
	INIT_PROPERTY_FIELD(offsetY);
	INIT_PROPERTY_FIELD(font);
	INIT_PROPERTY_FIELD(fontSize);
	INIT_PROPERTY_FIELD(labelText);
	INIT_PROPERTY_FIELD(textColor);
	INIT_PROPERTY_FIELD(outlineColor);
	INIT_PROPERTY_FIELD(outlineEnabled);
	INIT_PROPERTY_FIELD(sourceNode);

	// Automatically connect to the selected object node.
	setSourceNode(dynamic_object_cast<ObjectNode>(dataset->selection()->front()));
}

/******************************************************************************
* This method asks the overlay to paint its contents over the given viewport.
******************************************************************************/
void TextLabelOverlay::render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings)
{
	FloatType fontSize = this->fontSize() * renderSettings->outputImageHeight();
	if(fontSize <= 0) return;

	QPointF origin(offsetX() * renderSettings->outputImageWidth(), -offsetY() * renderSettings->outputImageHeight());
	FloatType margin = fontSize;

	QString textString = labelText();

	// Resolve attributes referenced in text string.
	if(sourceNode()) {
		const PipelineFlowState& flowState = sourceNode()->evaluatePipelineImmediately(PipelineEvalRequest(dataset()->animationSettings()->time(), false));
		for(auto a = flowState.attributes().cbegin(); a != flowState.attributes().cend(); ++a) {
			textString.replace(QStringLiteral("[") + a.key() + QStringLiteral("]"), a.value().toString());
		}
	}

	QRectF textRect(margin, margin, renderSettings->outputImageWidth() - margin*2, renderSettings->outputImageHeight() - margin*2);

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);

	QFont font = this->font();
	font.setPointSizeF(fontSize);
	painter.setFont(font);

	QPainterPath textPath = QPainterPath();
	textPath.addText(origin, font, textString);
	QRectF textBounds = textPath.boundingRect();

	if(alignment() & Qt::AlignLeft) textPath.translate(textRect.left(), 0);
	else if(alignment() & Qt::AlignRight) textPath.translate(textRect.right() - textBounds.width(), 0);
	else if(alignment() & Qt::AlignHCenter) textPath.translate(textRect.left() + textRect.width()/2.0 - textBounds.width()/2.0, 0);
	if(alignment() & Qt::AlignTop) textPath.translate(0, textRect.top() + textBounds.height());
	else if(alignment() & Qt::AlignBottom) textPath.translate(0, textRect.bottom());
	else if(alignment() & Qt::AlignVCenter) textPath.translate(0, textRect.top() + textRect.height()/2.0 + textBounds.height()/2.0);

	if(outlineEnabled()) {
		// Always render the outline pen 3 pixels wide, irrespective of frame buffer resolution.
		qreal outlineWidth = 3.0 / painter.combinedTransform().m11();
		painter.setPen(QPen(QBrush(outlineColor()), outlineWidth));
		painter.drawPath(textPath);
	}
	painter.fillPath(textPath, QBrush(textColor()));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
