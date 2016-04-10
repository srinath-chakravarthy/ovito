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

#include <plugins/particles/Particles.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include <core/dataset/DataSet.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/pipeline/PipelineObject.h>
#include "ColorLegendOverlay.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ColorLegendOverlay, ViewportOverlay);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _alignment, "Alignment", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _orientation, "Orientation", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _legendSize, "Size", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _font, "Font", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _fontSize, "FontSize", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _offsetX, "OffsetX");
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _offsetY, "OffsetY");
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _title, "Title");
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _label1, "Label1");
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _label2, "Label2");
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _aspectRatio, "AspectRatio", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ColorLegendOverlay, _valueFormatString, "ValueFormatString");
DEFINE_FLAGS_REFERENCE_FIELD(ColorLegendOverlay, _modifier, "Modifier", ColorCodingModifier, PROPERTY_FIELD_NO_SUB_ANIM);
DEFINE_FLAGS_PROPERTY_FIELD(ColorLegendOverlay, _textColor, "TextColor", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _alignment, "Position");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _orientation, "Orientation");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _legendSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _font, "Font");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _fontSize, "Font size");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _offsetY, "Offset Y");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _aspectRatio, "Aspect ratio");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _textColor, "Font color");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _title, "Title");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _label1, "Label 1");
SET_PROPERTY_FIELD_LABEL(ColorLegendOverlay, _label2, "Label 2");
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, _offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(ColorLegendOverlay, _offsetY, PercentParameterUnit);

/******************************************************************************
* Constructor.
******************************************************************************/
ColorLegendOverlay::ColorLegendOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_alignment(Qt::AlignHCenter | Qt::AlignBottom), _orientation(Qt::Horizontal),
		_legendSize(0.3), _offsetX(0), _offsetY(0),
		_fontSize(0.1), _valueFormatString("%g"), _aspectRatio(8.0),
		_textColor(0,0,0)
{
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_alignment);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_orientation);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_legendSize);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_offsetX);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_offsetY);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_aspectRatio);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_font);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_fontSize);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_title);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_label1);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_label2);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_valueFormatString);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_modifier);
	INIT_PROPERTY_FIELD(ColorLegendOverlay::_textColor);

	// Find a ColorCodingModifiers in the scene that we can connect to.
	dataset->sceneRoot()->visitObjectNodes([this](ObjectNode* node) {
		DataObject* obj = node->dataProvider();
		while(obj) {
			if(PipelineObject* pipeline = dynamic_object_cast<PipelineObject>(obj)) {
				for(ModifierApplication* modApp : pipeline->modifierApplications()) {
					if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
						setModifier(mod);
						if(mod->isEnabled())
							return false;	// Stop search.
					}
				}
				obj = pipeline->sourceObject();
			}
			else break;
		}
		return true;
	});
}

/******************************************************************************
* This method asks the overlay to paint its contents over the given viewport.
******************************************************************************/
void ColorLegendOverlay::render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings)
{
	if(!modifier()) return;

	FloatType legendSize = _legendSize.value() * renderSettings->outputImageHeight();
	if(legendSize <= 0) return;

	FloatType colorBarWidth = legendSize;
	FloatType colorBarHeight = colorBarWidth / std::max(FloatType(0.01), _aspectRatio.value());
	bool vertical = (_orientation.value() == Qt::Vertical);
	if(vertical)
		std::swap(colorBarWidth, colorBarHeight);

	QPointF origin(_offsetX.value() * renderSettings->outputImageWidth(), -_offsetY.value() * renderSettings->outputImageHeight());
	FloatType hmargin = 0.01 * renderSettings->outputImageWidth();
	FloatType vmargin = 0.01 * renderSettings->outputImageHeight();

	if(_alignment.value() & Qt::AlignLeft) origin.rx() += hmargin;
	else if(_alignment.value() & Qt::AlignRight) origin.rx() += renderSettings->outputImageWidth() - hmargin - colorBarWidth;
	else if(_alignment.value() & Qt::AlignHCenter) origin.rx() += 0.5 * renderSettings->outputImageWidth() - 0.5 * colorBarWidth;

	if(_alignment.value() & Qt::AlignTop) origin.ry() += vmargin;
	else if(_alignment.value() & Qt::AlignBottom) origin.ry() += renderSettings->outputImageHeight() - vmargin - colorBarHeight;
	else if(_alignment.value() & Qt::AlignVCenter) origin.ry() += 0.5 * renderSettings->outputImageHeight() - 0.5 * colorBarHeight;

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

	// Create the color scale image.
	int imageSize = 256;
	QImage image(vertical ? 1 : imageSize, vertical ? imageSize : 1, QImage::Format_RGB32);
	for(int i = 0; i < imageSize; i++) {
		FloatType t = (FloatType)i / (FloatType)(imageSize - 1);
		Color color = modifier()->colorGradient()->valueToColor(vertical ? (FloatType(1) - t) : t);
		image.setPixel(vertical ? 0 : i, vertical ? i : 0, QColor(color).rgb());
	}
	painter.drawImage(QRectF(origin, QSizeF(colorBarWidth, colorBarHeight)), image);

	qreal fontSize = legendSize * std::max(0.0, (double)_fontSize.value());
	if(fontSize == 0) return;
	QFont font = _font.value();
	painter.setPen((QColor)_textColor.value());

	// Get modifier's parameters.
	FloatType startValue = modifier()->startValue();
	FloatType endValue = modifier()->endValue();

	QByteArray format = valueFormatString().toLatin1();
	if(format.contains("%s")) format.clear();

	QString titleLabel, topLabel, bottomLabel;
	if(label1().isEmpty())
		topLabel.sprintf(format.constData(), endValue);
	else
		topLabel = label1();
	if(label2().isEmpty())
		bottomLabel.sprintf(format.constData(), startValue);
	else
		bottomLabel = label2();
	if(title().isEmpty())
		titleLabel = modifier()->sourceProperty().nameWithComponent();
	else
		titleLabel = title();

	font.setPointSizeF(fontSize);
	painter.setFont(font);

	qreal textMargin = 0.2 * legendSize / std::max(FloatType(0.01), _aspectRatio.value());
	if(!vertical || (_alignment.value() & Qt::AlignHCenter)) {
		painter.drawText(QRectF(origin.x() + 0.5 * colorBarWidth, origin.y() - 0.5 * textMargin, 0, 0), Qt::AlignHCenter | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, titleLabel);
	}
	else {
		if(_alignment.value() & Qt::AlignLeft)
			painter.drawText(QRectF(origin.x(), origin.y() - textMargin, 0, 0), Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, titleLabel);
		else if(_alignment.value() & Qt::AlignRight)
			painter.drawText(QRectF(origin.x() + colorBarWidth, origin.y() - textMargin, 0, 0), Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, titleLabel);
	}

	font.setPointSizeF(fontSize * 0.8);
	painter.setFont(font);

	if(!vertical) {
		painter.drawText(QRectF(origin.x() - textMargin, origin.y() + 0.5 * colorBarHeight, 0, 0), Qt::AlignRight | Qt::AlignVCenter | Qt::TextDontClip | Qt::TextSingleLine, bottomLabel);
		painter.drawText(QRectF(origin.x() + colorBarWidth + textMargin, origin.y() + 0.5 * colorBarHeight, 0, 0), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip | Qt::TextSingleLine, topLabel);
	}
	else {
		if(_alignment.value() & Qt::AlignLeft) {
			painter.drawText(QRectF(origin.x() + colorBarWidth + textMargin, origin.y(), 0, 0), Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine, topLabel);
			painter.drawText(QRectF(origin.x() + colorBarWidth + textMargin, origin.y() + colorBarHeight, 0, 0), Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, bottomLabel);
		}
		else if(_alignment.value() & Qt::AlignRight) {
			painter.drawText(QRectF(origin.x() - textMargin, origin.y(), 0, 0), Qt::AlignRight | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine, topLabel);
			painter.drawText(QRectF(origin.x() - textMargin, origin.y() + colorBarHeight, 0, 0), Qt::AlignRight | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, bottomLabel);
		}
		else if(_alignment.value() & Qt::AlignHCenter) {
			painter.drawText(QRectF(origin.x() + colorBarWidth + textMargin, origin.y(), 0, 0), Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip | Qt::TextSingleLine, topLabel);
			painter.drawText(QRectF(origin.x() + colorBarWidth + textMargin, origin.y() + colorBarHeight, 0, 0), Qt::AlignLeft | Qt::AlignBottom | Qt::TextDontClip | Qt::TextSingleLine, bottomLabel);
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
