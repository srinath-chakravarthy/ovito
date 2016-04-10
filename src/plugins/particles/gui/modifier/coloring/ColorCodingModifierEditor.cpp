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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/coloring/ColorCodingModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dialogs/LoadImageFileDialog.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/CustomParameterUI.h>
#include <gui/dialogs/SaveImageFileDialog.h>
#include <core/plugins/PluginManager.h>
#include "ColorCodingModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ColorCodingModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ColorCodingModifier, ColorCodingModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ColorCodingModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Color coding"), rolloutParams, "particles.modifiers.color_coding.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(2);

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ColorCodingModifier::_sourceProperty));
	layout1->addWidget(new QLabel(tr("Property:"), rollout));
	layout1->addWidget(sourcePropertyUI->comboBox());

	colorGradientList = new QComboBox(rollout);
	layout1->addWidget(new QLabel(tr("Color gradient:"), rollout));
	layout1->addWidget(colorGradientList);
	colorGradientList->setIconSize(QSize(48,16));
	connect(colorGradientList, (void (QComboBox::*)(int))&QComboBox::activated, this, &ColorCodingModifierEditor::onColorGradientSelected);
	for(const OvitoObjectType* clazz : PluginManager::instance().listClasses(ColorCodingGradient::OOType)) {
		if(clazz == &ColorCodingImageGradient::OOType)
			continue;
		colorGradientList->addItem(iconFromColorMapClass(clazz), clazz->displayName(), QVariant::fromValue(clazz));
	}
	colorGradientList->insertSeparator(colorGradientList->count());
	colorGradientList->addItem(tr("Load custom color map..."));
	_gradientListContainCustomItem = false;

	// Update color legend if another modifier has been loaded into the editor.
	connect(this, &ColorCodingModifierEditor::contentsReplaced, this, &ColorCodingModifierEditor::updateColorGradient);

	layout1->addSpacing(10);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	// End value parameter.
	FloatParameterUI* endValuePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorCodingModifier::_endValueCtrl));
	layout2->addWidget(endValuePUI->label(), 0, 0);
	layout2->addLayout(endValuePUI->createFieldLayout(), 0, 1);

	// Insert color legend display.
	colorLegendLabel = new QLabel(rollout);
	colorLegendLabel->setScaledContents(true);
	layout2->addWidget(colorLegendLabel, 1, 1);

	// Start value parameter.
	FloatParameterUI* startValuePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorCodingModifier::_startValueCtrl));
	layout2->addWidget(startValuePUI->label(), 2, 0);
	layout2->addLayout(startValuePUI->createFieldLayout(), 2, 1);

	// Export color scale button.
	QToolButton* exportBtn = new QToolButton(rollout);
	exportBtn->setIcon(QIcon(":/particles/icons/export_color_scale.png"));
	exportBtn->setToolTip("Export color map to image file");
	exportBtn->setAutoRaise(true);
	exportBtn->setIconSize(QSize(42,22));
	connect(exportBtn, &QPushButton::clicked, this, &ColorCodingModifierEditor::onExportColorScale);
	layout2->addWidget(exportBtn, 1, 0, Qt::AlignCenter);

	layout1->addSpacing(8);
	QPushButton* adjustBtn = new QPushButton(tr("Adjust range"), rollout);
	connect(adjustBtn, &QPushButton::clicked, this, &ColorCodingModifierEditor::onAdjustRange);
	layout1->addWidget(adjustBtn);
	layout1->addSpacing(4);
	QPushButton* reverseBtn = new QPushButton(tr("Reverse range"), rollout);
	connect(reverseBtn, &QPushButton::clicked, this, &ColorCodingModifierEditor::onReverseRange);
	layout1->addWidget(reverseBtn);

	layout1->addSpacing(8);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(ColorCodingModifier::_colorOnlySelected));
	layout1->addWidget(onlySelectedPUI->checkBox());

	// Keep selection
	BooleanParameterUI* keepSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(ColorCodingModifier::_keepSelection));
	layout1->addWidget(keepSelectionPUI->checkBox());
	connect(onlySelectedPUI->checkBox(), &QCheckBox::toggled, keepSelectionPUI, &BooleanParameterUI::setEnabled);
	keepSelectionPUI->setEnabled(false);
}

/******************************************************************************
* Updates the display for the color gradient.
******************************************************************************/
void ColorCodingModifierEditor::updateColorGradient()
{
	ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(editObject());
	if(!mod) return;

	// Create the color legend image.
	int legendHeight = 128;
	QImage image(1, legendHeight, QImage::Format_RGB32);
	for(int y = 0; y < legendHeight; y++) {
		FloatType t = (FloatType)y / (legendHeight - 1);
		Color color = mod->colorGradient()->valueToColor(1.0 - t);
		image.setPixel(0, y, QColor(color).rgb());
	}
	colorLegendLabel->setPixmap(QPixmap::fromImage(image));

	// Select the right entry in the color gradient selector.
	bool isCustomMap = false;
	if(mod->colorGradient()) {
		int index = colorGradientList->findData(QVariant::fromValue(&mod->colorGradient()->getOOType()));
		if(index >= 0)
			colorGradientList->setCurrentIndex(index);
		else
			isCustomMap = true;
	}
	else colorGradientList->setCurrentIndex(-1);

	if(isCustomMap) {
		if(!_gradientListContainCustomItem) {
			_gradientListContainCustomItem = true;
			colorGradientList->insertItem(colorGradientList->count() - 2, iconFromColorMap(mod->colorGradient()), tr("Custom color map"));
			colorGradientList->insertSeparator(colorGradientList->count() - 3);
		}
		else {
			colorGradientList->setItemIcon(colorGradientList->count() - 3, iconFromColorMap(mod->colorGradient()));
		}
		colorGradientList->setCurrentIndex(colorGradientList->count() - 3);
	}
	else if(_gradientListContainCustomItem) {
		_gradientListContainCustomItem = false;
		colorGradientList->removeItem(colorGradientList->count() - 3);
		colorGradientList->removeItem(colorGradientList->count() - 3);
	}
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ColorCodingModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ReferenceChanged &&
			static_cast<ReferenceFieldEvent*>(event)->field() == PROPERTY_FIELD(ColorCodingModifier::_colorGradient)) {
		updateColorGradient();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the user selects a color gradient in the list box.
******************************************************************************/
void ColorCodingModifierEditor::onColorGradientSelected(int index)
{
	if(index < 0) return;
	ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(editObject());
	OVITO_CHECK_OBJECT_POINTER(mod);

	const OvitoObjectType* descriptor = colorGradientList->itemData(index).value<const OvitoObjectType*>();
	if(descriptor) {
		undoableTransaction(tr("Change color gradient"), [descriptor, mod]() {
			OORef<ColorCodingGradient> gradient = static_object_cast<ColorCodingGradient>(descriptor->createInstance(mod->dataset()));
			if(gradient) {
				mod->setColorGradient(gradient);

				QSettings settings;
				settings.beginGroup(ColorCodingModifier::OOType.plugin()->pluginId());
				settings.beginGroup(ColorCodingModifier::OOType.name());
				settings.setValue(PROPERTY_FIELD(ColorCodingModifier::_colorGradient).identifier(),
						QVariant::fromValue(OvitoObjectType::encodeAsString(descriptor)));
			}
		});
	}
	else if(index == colorGradientList->count() - 1) {
		undoableTransaction(tr("Change color gradient"), [this, mod]() {
			LoadImageFileDialog fileDialog(container(), tr("Pick color map image"));
			if(fileDialog.exec()) {
				OORef<ColorCodingImageGradient> gradient(new ColorCodingImageGradient(mod->dataset()));
				gradient->loadImage(fileDialog.imageInfo().filename());
				mod->setColorGradient(gradient);
			}
		});
	}
}

/******************************************************************************
* Is called when the user presses the "Adjust Range" button.
******************************************************************************/
void ColorCodingModifierEditor::onAdjustRange()
{
	ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(editObject());
	OVITO_CHECK_OBJECT_POINTER(mod);

	undoableTransaction(tr("Adjust range"), [mod]() {
		mod->adjustRange();
	});
}

/******************************************************************************
* Is called when the user presses the "Reverse Range" button.
******************************************************************************/
void ColorCodingModifierEditor::onReverseRange()
{
	ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(editObject());

	if(mod->startValueController() && mod->endValueController()) {
		undoableTransaction(tr("Reverse range"), [mod]() {

			// Swap controllers for start and end value.
			OORef<Controller> oldStartValue = mod->startValueController();
			mod->setStartValueController(mod->endValueController());
			mod->setEndValueController(oldStartValue);

		});
	}
}

/******************************************************************************
* Is called when the user presses the "Export color scale" button.
******************************************************************************/
void ColorCodingModifierEditor::onExportColorScale()
{
	ColorCodingModifier* mod = static_object_cast<ColorCodingModifier>(editObject());
	if(!mod || !mod->colorGradient()) return;

	SaveImageFileDialog fileDialog(colorLegendLabel, tr("Save color map"));
	if(fileDialog.exec()) {

		// Create the color legend image.
		int legendWidth = 32;
		int legendHeight = 256;
		QImage image(1, legendHeight, QImage::Format_RGB32);
		for(int y = 0; y < legendHeight; y++) {
			FloatType t = (FloatType)y / (FloatType)(legendHeight - 1);
			Color color = mod->colorGradient()->valueToColor(1.0 - t);
			image.setPixel(0, y, QColor(color).rgb());
		}

		QString imageFilename = fileDialog.imageInfo().filename();
		if(!image.scaled(legendWidth, legendHeight, Qt::IgnoreAspectRatio, Qt::FastTransformation).save(imageFilename, fileDialog.imageInfo().format())) {
			Exception ex(tr("Failed to save image to file '%1'.").arg(imageFilename));
			ex.showError();
		}
	}
}

/******************************************************************************
* Returns an icon representing the given color map class.
******************************************************************************/
QIcon ColorCodingModifierEditor::iconFromColorMapClass(const OvitoObjectType* clazz)
{
	/// Cache icons for color map types.
	static std::map<const OvitoObjectType*, QIcon> iconCache;
	auto entry = iconCache.find(clazz);
	if(entry != iconCache.end())
		return entry->second;

	DataSet* dataset = mainWindow()->datasetContainer().currentSet();
	OVITO_ASSERT(dataset);
	if(dataset) {
		try {
			// Create a temporary instance of the color map class.
			OORef<ColorCodingGradient> map = static_object_cast<ColorCodingGradient>(clazz->createInstance(dataset));
			if(map) {
				QIcon icon = iconFromColorMap(map);
				iconCache.insert(std::make_pair(clazz, icon));
				return icon;
			}
		}
		catch(...) {}
	}
	return QIcon();
}

/******************************************************************************
* Returns an icon representing the given color map.
******************************************************************************/
QIcon ColorCodingModifierEditor::iconFromColorMap(ColorCodingGradient* map)
{
	const int sizex = 48;
	const int sizey = 16;
	QImage image(sizex, sizey, QImage::Format_RGB32);
	for(int x = 0; x < sizex; x++) {
		FloatType t = (FloatType)x / (sizex - 1);
		uint c = QColor(map->valueToColor(t)).rgb();
		for(int y = 0; y < sizey; y++)
			image.setPixel(x, y, c);
	}
	return QIcon(QPixmap::fromImage(image));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
