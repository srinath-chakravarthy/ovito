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

#if 0
#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, TextLabelOverlay, ViewportOverlay);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _alignment, "Alignment", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _font, "Font", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _fontSize, "FontSize", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _offsetX, "OffsetX", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _offsetY, "OffsetY", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(TextLabelOverlay, _labelText, "LabelText");
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _textColor, "TextColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _outlineColor, "OutlineColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TextLabelOverlay, _outlineEnabled, "OutlineEnabled", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(TextLabelOverlay, _sourceNode, "SourceNode", ObjectNode, PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _alignment, "Position");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _font, "Font");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _textColor, "Text color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _outlineColor, "Outline color");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _outlineEnabled, "Enable outline");
SET_PROPERTY_FIELD_LABEL(TextLabelOverlay, _sourceNode, "Attributes source");
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, _offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(TextLabelOverlay, _offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TextLabelOverlay, _fontSize, FloatParameterUnit, 0);

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
	INIT_PROPERTY_FIELD(TextLabelOverlay::_alignment);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_offsetX);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_offsetY);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_font);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_fontSize);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_labelText);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_textColor);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_outlineColor);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_outlineEnabled);
	INIT_PROPERTY_FIELD(TextLabelOverlay::_sourceNode);

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

	QPointF origin(_offsetX.value() * renderSettings->outputImageWidth(), -_offsetY.value() * renderSettings->outputImageHeight());
	FloatType margin = fontSize;

	QString textString = labelText();

	// Resolve attributes referenced in text string.
	if(sourceNode()) {
		const PipelineFlowState& flowState = sourceNode()->evalPipeline(dataset()->animationSettings()->time());
		for(auto a = flowState.attributes().cbegin(); a != flowState.attributes().cend(); ++a) {
			textString.replace(QStringLiteral("[") + a.key() + QStringLiteral("]"), a.value().toString());
		}
	}

	QRectF textRect(margin, margin, renderSettings->outputImageWidth() - margin*2, renderSettings->outputImageHeight() - margin*2);

	int flags = Qt::TextDontClip;
	if(_alignment.value() & Qt::AlignLeft) flags |= Qt::AlignLeft;
	else if(_alignment.value() & Qt::AlignRight) flags |= Qt::AlignRight;
	else if(_alignment.value() & Qt::AlignHCenter) flags |= Qt::AlignHCenter;
	if(_alignment.value() & Qt::AlignTop) flags |= Qt::AlignTop;
	else if(_alignment.value() & Qt::AlignBottom) flags |= Qt::AlignBottom;
	else if(_alignment.value() & Qt::AlignVCenter) flags |= Qt::AlignVCenter;

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	QPen pen(textColor());
	painter.setPen(pen);

	QFont font = this->font();
	font.setPointSizeF(fontSize);
	painter.setFont(font);

	painter.drawText(textRect.normalized().translated(origin), flags, textString);

#if 0
	QTextDocument doc(textString);
	doc.setDefaultFont(font);
	QTextCharFormat format;
	format.setTextOutline(QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); // Color and width of outline
	QTextCursor cursor(&doc);
	cursor.select(QTextCursor::Document);
	cursor.mergeCharFormat(format);
	doc.drawContents(&painter);
#endif
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
