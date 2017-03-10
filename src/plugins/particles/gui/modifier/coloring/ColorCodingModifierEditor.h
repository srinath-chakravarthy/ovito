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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <plugins/particles/modifier/coloring/ColorCodingModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the ColorCodingModifier class.
 */
class ColorCodingModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	/// Returns an icon representing the given color map class.
	QIcon iconFromColorMapClass(const OvitoObjectType* clazz);

	/// Returns an icon representing the given color map.
	QIcon iconFromColorMap(ColorCodingGradient* map);

	/// The list of available color gradients.
	QComboBox* colorGradientList;

	/// Indicates the combo box already contains an item for a custom color map.
	bool _gradientListContainCustomItem;

	/// Label that displays the color gradient picture.
	QLabel* colorLegendLabel;

protected Q_SLOTS:

	/// Updates the display for the color gradient.
	void updateColorGradient();

	/// Is called when the user selects a color gradient in the list box.
	void onColorGradientSelected(int index);

	/// Is called when the user presses the "Adjust range" button.
	void onAdjustRange();

	/// Is called when the user presses the "Adjust range over all frames" button.
	void onAdjustRangeGlobal();

	/// Is called when the user presses the "Reverse range" button.
	void onReverseRange();

	/// Is called when the user presses the "Export color scale" button.
	void onExportColorScale();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


