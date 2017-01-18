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

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include "CoordinateTripodOverlay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CoordinateTripodOverlay, ViewportOverlay);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _alignment, "Alignment", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _tripodSize, "Size", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _lineWidth, "LineWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _font, "Font", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _fontSize, "FontSize", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _offsetX, "OffsetX", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _offsetY, "OffsetY", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis1Enabled, "Axis1Enabled");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis2Enabled, "Axis2Enabled");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis3Enabled, "Axis3Enabled");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis4Enabled, "Axis4Enabled");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis1Label, "Axis1Label");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis2Label, "Axis2Label");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis3Label, "Axis3Label");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis4Label, "Axis4Label");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis1Dir, "Axis1Dir");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis2Dir, "Axis2Dir");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis3Dir, "Axis3Dir");
DEFINE_PROPERTY_FIELD(CoordinateTripodOverlay, _axis4Dir, "Axis4Dir");
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _axis1Color, "Axis1Color", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _axis2Color, "Axis2Color", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _axis3Color, "Axis3Color", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinateTripodOverlay, _axis4Color, "Axis4Color", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _alignment, "Position");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _tripodSize, "Size factor");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _lineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _font, "Font");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _fontSize, "Label size");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _offsetX, "Offset X");
SET_PROPERTY_FIELD_LABEL(CoordinateTripodOverlay, _offsetY, "Offset Y");
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, _offsetX, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS(CoordinateTripodOverlay, _offsetY, PercentParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, _tripodSize, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, _lineWidth, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinateTripodOverlay, _fontSize, FloatParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
CoordinateTripodOverlay::CoordinateTripodOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_alignment(Qt::AlignLeft | Qt::AlignBottom),
		_tripodSize(0.075), _lineWidth(0.06), _offsetX(0), _offsetY(0),
		_fontSize(0.4),
		_axis1Enabled(true), _axis2Enabled(true), _axis3Enabled(true), _axis4Enabled(false),
		_axis1Label("x"), _axis2Label("y"), _axis3Label("z"), _axis4Label("w"),
		_axis1Dir(1,0,0), _axis2Dir(0,1,0), _axis3Dir(0,0,1), _axis4Dir(sqrt(0.5),sqrt(0.5),0),
		_axis1Color(1,0,0), _axis2Color(0,0.8,0), _axis3Color(0.2,0.2,1), _axis4Color(1,0,1)
{
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_alignment);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_tripodSize);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_lineWidth);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_offsetX);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_offsetY);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_font);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_fontSize);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis1Enabled);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis2Enabled);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis3Enabled);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis4Enabled);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis1Label);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis2Label);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis3Label);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis4Label);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis1Dir);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis2Dir);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis3Dir);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis4Dir);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis1Color);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis2Color);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis3Color);
	INIT_PROPERTY_FIELD(CoordinateTripodOverlay::_axis4Color);
}

/******************************************************************************
* This method asks the overlay to paint its contents over the given viewport.
******************************************************************************/
void CoordinateTripodOverlay::render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings)
{
	FloatType tripodSize = _tripodSize.value() * renderSettings->outputImageHeight();
	if(tripodSize <= 0) return;

	FloatType lineWidth = _lineWidth.value() * tripodSize;
	if(lineWidth <= 0) return;

	FloatType arrowSize = 0.17f;

	QPointF origin(_offsetX.value() * renderSettings->outputImageWidth(), -_offsetY.value() * renderSettings->outputImageHeight());
	FloatType margin = tripodSize + lineWidth;

	if(_alignment.value() & Qt::AlignLeft) origin.rx() += margin;
	else if(_alignment.value() & Qt::AlignRight) origin.rx() += renderSettings->outputImageWidth() - margin;
	else if(_alignment.value() & Qt::AlignHCenter) origin.rx() += 0.5 * renderSettings->outputImageWidth();

	if(_alignment.value() & Qt::AlignTop) origin.ry() += margin;
	else if(_alignment.value() & Qt::AlignBottom) origin.ry() += renderSettings->outputImageHeight() - margin;
	else if(_alignment.value() & Qt::AlignVCenter) origin.ry() += 0.5 * renderSettings->outputImageHeight();

	// Project axes to screen.
	Vector3 axisDirs[4] = {
			projParams.viewMatrix * _axis1Dir.value(),
			projParams.viewMatrix * _axis2Dir.value(),
			projParams.viewMatrix * _axis3Dir.value(),
			projParams.viewMatrix * _axis4Dir.value()
	};

	// Get axis colors.
	QColor axisColors[4] = {
			_axis1Color.value(),
			_axis2Color.value(),
			_axis3Color.value(),
			_axis4Color.value()
	};

	// Order axes back to front.
	std::vector<int> orderedAxes;
	if(_axis1Enabled) orderedAxes.push_back(0);
	if(_axis2Enabled) orderedAxes.push_back(1);
	if(_axis3Enabled) orderedAxes.push_back(2);
	if(_axis4Enabled) orderedAxes.push_back(3);
	std::sort(orderedAxes.begin(), orderedAxes.end(), [&axisDirs](int a, int b) {
		return axisDirs[a].z() < axisDirs[b].z();
	});

	QString labels[4] = {
			_axis1Label.value(),
			_axis2Label.value(),
			_axis3Label.value(),
			_axis4Label.value()
	};
	QFont font = _font.value();
	qreal fontSize = tripodSize * std::max(0.0, (double)_fontSize.value());
	if(fontSize != 0) {
		font.setPointSizeF(fontSize);
		painter.setFont(font);
	}

	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);
	for(int axis : orderedAxes) {
		QBrush brush(axisColors[axis]);
		QPen pen(axisColors[axis]);
		pen.setWidthF(lineWidth);
		pen.setJoinStyle(Qt::MiterJoin);
		pen.setCapStyle(Qt::FlatCap);
		painter.setPen(pen);
		painter.setBrush(brush);
		Vector3 dir = tripodSize * axisDirs[axis];
		Vector2 dir2(dir.x(), dir.y());
		if(dir2.squaredLength() > FLOATTYPE_EPSILON) {
			painter.drawLine(origin, origin + QPointF(dir2.x(), -dir2.y()));
			Vector2 ndir = dir2;
			if(ndir.length() > arrowSize * tripodSize)
				ndir.resize(arrowSize * tripodSize);
			QPointF head[3];
			head[1] = origin + QPointF(dir2.x(), -dir2.y());
			head[0] = head[1] + QPointF(0.5 *  ndir.y() - ndir.x(), -(0.5 * -ndir.x() - ndir.y()));
			head[2] = head[1] + QPointF(0.5 * -ndir.y() - ndir.x(), -(0.5 *  ndir.x() - ndir.y()));
			painter.drawConvexPolygon(head, 3);
		}

		if(fontSize != 0) {
			QRectF textRect = painter.boundingRect(QRectF(0,0,0,0), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
			textRect.translate(origin + QPointF(dir.x(), -dir.y()));
			if(std::abs(dir.x()) > FLOATTYPE_EPSILON || std::abs(dir.y()) > FLOATTYPE_EPSILON) {
				FloatType offset1 = dir.x() != 0 ? textRect.width() / std::abs(dir.x()) : FLOATTYPE_MAX;
				FloatType offset2 = dir.y() != 0 ? textRect.height() / std::abs(dir.y()) : FLOATTYPE_MAX;
				textRect.translate(0.5 * std::min(offset1, offset2) * QPointF(dir.x(), -dir.y()));
				Vector3 ndir(dir.x(), dir.y(), 0);
				ndir.resize(lineWidth);
				textRect.translate(ndir.x(), -ndir.y());
			}
			painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextDontClip, labels[axis]);
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
