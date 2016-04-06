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

#ifndef __OVITO_COORDINATE_TRIPOD_OVERLAY_H
#define __OVITO_COORDINATE_TRIPOD_OVERLAY_H

#include <core/Core.h>
#include <core/gui/properties/PropertiesEditor.h>
#include "ViewportOverlay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A viewport overlay that displays the coordinate system orientation.
 */
class OVITO_CORE_EXPORT CoordinateTripodOverlay : public ViewportOverlay
{
public:

	/// \brief Constructor.
	Q_INVOKABLE CoordinateTripodOverlay(DataSet* dataset);

	/// \brief This method asks the overlay to paint its contents over the given viewport.
	virtual void render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings) override;

	/// Moves the position of the overlay in the viewport by the given amount,
	/// which is specified as a fraction of the viewport render size.
	virtual void moveOverlayInViewport(const Vector2& delta) override {
		setOffsetX(offsetX() + delta.x());
		setOffsetY(offsetY() + delta.y());
	}

	/// Returns the corner of the viewport where the tripod is shown.
	int alignment() const { return _alignment; }

	/// Sets the corner of the viewport where the tripod is shown.
	void setAlignment(int align) { _alignment = align; }

	/// Returns the size of the tripod.
	FloatType tripodSize() const { return _tripodSize; }

	/// Sets the size of the tripod.
	void setTripodSize(FloatType size) { _tripodSize = size; }

	/// Returns the line width.
	FloatType lineWidth() const { return _lineWidth; }

	/// Sets the line width.
	void setLineWidth(FloatType lw) { _lineWidth = lw; }

	/// Returns the horizontal offset of the tripod's position.
	FloatType offsetX() const { return _offsetX; }

	/// Sets the horizontal offset of the tripod's position.
	void setOffsetX(FloatType offset) { _offsetX = offset; }

	/// Returns the vertical offset of the tripod's position.
	FloatType offsetY() const { return _offsetY; }

	/// Sets the vertical offset of the tripod's position.
	void setOffsetY(FloatType offset) { _offsetY = offset; }

	/// Returns the text label font.
	const QFont& font() const { return _font; }

	/// Sets the text label font.
	void setFont(const QFont& font) { _font = font; }

	/// Returns the text label font size.
	FloatType fontSize() const { return _fontSize; }

	/// Sets the text label font size.
	void setFontSize(FloatType fontSize) { _fontSize = fontSize; }

	/// Returns whether the first axis is displayed.
	bool axis1Enabled() const { return _axis1Enabled; }

	/// Sets whether display is enabled for the first axis.
	void setAxis1Enabled(bool enabled) { _axis1Enabled = enabled; }

	/// Returns whether the second axis is displayed.
	bool axis2Enabled() const { return _axis2Enabled; }

	/// Sets whether display is enabled for the second axis.
	void setAxis2Enabled(bool enabled) { _axis2Enabled = enabled; }

	/// Returns whether the third axis is displayed.
	bool axis3Enabled() const { return _axis3Enabled; }

	/// Sets whether display is enabled for the third  axis.
	void setAxis3Enabled(bool enabled) { _axis3Enabled = enabled; }

	/// Returns whether the fourth axis is displayed.
	bool axis4Enabled() const { return _axis4Enabled; }

	/// Sets whether display is enabled for the fourth axis.
	void setAxis4Enabled(bool enabled) { _axis4Enabled = enabled; }

	/// Returns the text label of the first axis.
	const QString& axis1Label() const { return _axis1Label; }

	/// Sets the text label of the first axis.
	void setAxis1Label(const QString& label) { _axis1Label = label; }

	/// Returns the text label of the second axis.
	const QString& axis2Label() const { return _axis2Label; }

	/// Sets the text label of the second axis.
	void setAxis2Label(const QString& label) { _axis2Label = label; }

	/// Returns the text label of the third axis.
	const QString& axis3Label() const { return _axis3Label; }

	/// Sets the text label of the third axis.
	void setAxis3Label(const QString& label) { _axis3Label = label; }

	/// Returns the text label of the fourth axis.
	const QString& axis4Label() const { return _axis4Label; }

	/// Sets the text label of the fourth axis.
	void setAxis4Label(const QString& label) { _axis4Label = label; }

	/// Returns the direction of the first axis.
	const Vector3& axis1Dir() const { return _axis1Dir; }

	/// Sets the direction of the first axis.
	void setAxis1Dir(const Vector3& dir) { _axis1Dir = dir; }

	/// Returns the direction of the second axis.
	const Vector3& axis2Dir() const { return _axis2Dir; }

	/// Sets the direction of the second axis.
	void setAxis2Dir(const Vector3& dir) { _axis2Dir = dir; }

	/// Returns the direction of the third axis.
	const Vector3& axis3Dir() const { return _axis3Dir; }

	/// Sets the direction of the third axis.
	void setAxis3Dir(const Vector3& dir) { _axis3Dir = dir; }

	/// Returns the direction of the fourth axis.
	const Vector3& axis4Dir() const { return _axis4Dir; }

	/// Sets the direction of the fourth axis.
	void setAxis4Dir(const Vector3& dir) { _axis4Dir = dir; }

private:

	/// The corner of the viewport where the tripod is shown in.
	PropertyField<int> _alignment;

	/// Controls the size of the tripod.
	PropertyField<FloatType> _tripodSize;

	/// Controls the line width.
	PropertyField<FloatType> _lineWidth;

	/// Controls the horizontal offset of tripod position.
	PropertyField<FloatType> _offsetX;

	/// Controls the vertical offset of tripod position.
	PropertyField<FloatType> _offsetY;

	/// Controls the label font.
	PropertyField<QFont> _font;

	/// Controls the label font size.
	PropertyField<FloatType> _fontSize;

	/// Controls the display of the first axis.
	PropertyField<bool> _axis1Enabled;

	/// Controls the display of the second axis.
	PropertyField<bool> _axis2Enabled;

	/// Controls the display of the third axis.
	PropertyField<bool> _axis3Enabled;

	/// Controls the display of the fourth axis.
	PropertyField<bool> _axis4Enabled;

	/// The label of the first axis.
	PropertyField<QString> _axis1Label;

	/// The label of the second axis.
	PropertyField<QString> _axis2Label;

	/// The label of the third axis.
	PropertyField<QString> _axis3Label;

	/// The label of the fourth axis.
	PropertyField<QString> _axis4Label;

	/// The direction of the first axis.
	PropertyField<Vector3> _axis1Dir;

	/// The direction of the second axis.
	PropertyField<Vector3> _axis2Dir;

	/// The direction of the third axis.
	PropertyField<Vector3> _axis3Dir;

	/// The direction of the fourth axis.
	PropertyField<Vector3> _axis4Dir;

	/// The display color of the first axis.
	PropertyField<Color, QColor> _axis1Color;

	/// The display color of the second axis.
	PropertyField<Color, QColor> _axis2Color;

	/// The display color of the third axis.
	PropertyField<Color, QColor> _axis3Color;

	/// The display color of the fourth axis.
	PropertyField<Color, QColor> _axis4Color;

	DECLARE_PROPERTY_FIELD(_alignment);
	DECLARE_PROPERTY_FIELD(_font);
	DECLARE_PROPERTY_FIELD(_fontSize);
	DECLARE_PROPERTY_FIELD(_tripodSize);
	DECLARE_PROPERTY_FIELD(_lineWidth);
	DECLARE_PROPERTY_FIELD(_offsetX);
	DECLARE_PROPERTY_FIELD(_offsetY);
	DECLARE_PROPERTY_FIELD(_axis1Enabled);
	DECLARE_PROPERTY_FIELD(_axis2Enabled);
	DECLARE_PROPERTY_FIELD(_axis3Enabled);
	DECLARE_PROPERTY_FIELD(_axis4Enabled);
	DECLARE_PROPERTY_FIELD(_axis1Label);
	DECLARE_PROPERTY_FIELD(_axis2Label);
	DECLARE_PROPERTY_FIELD(_axis3Label);
	DECLARE_PROPERTY_FIELD(_axis4Label);
	DECLARE_PROPERTY_FIELD(_axis1Dir);
	DECLARE_PROPERTY_FIELD(_axis2Dir);
	DECLARE_PROPERTY_FIELD(_axis3Dir);
	DECLARE_PROPERTY_FIELD(_axis4Dir);
	DECLARE_PROPERTY_FIELD(_axis1Color);
	DECLARE_PROPERTY_FIELD(_axis2Color);
	DECLARE_PROPERTY_FIELD(_axis3Color);
	DECLARE_PROPERTY_FIELD(_axis4Color);

	Q_CLASSINFO("DisplayName", "Coordinate tripod");

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief A properties editor for the CoordinateTripodOverlay class.
 */
class CoordinateTripodOverlayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE CoordinateTripodOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_COORDINATE_TRIPOD_OVERLAY_H
