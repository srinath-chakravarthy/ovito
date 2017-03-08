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

#include <gui/GUI.h>
#include <gui/widgets/general/SpinnerWidget.h>
#include <gui/properties/PropertiesEditor.h>
#include <gui/properties/PropertiesPanel.h>
#include <gui/mainwin/MainWindow.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/importexport/FileExporter.h>
#include "FileExporterSettingsDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructor.
******************************************************************************/
FileExporterSettingsDialog::FileExporterSettingsDialog(MainWindow* mainWindow, FileExporter* exporter)
	: QDialog(mainWindow), _exporter(exporter)
{
	setWindowTitle(tr("Export Settings"));

	_mainLayout = new QVBoxLayout(this);
	QRadioButton* radioBtn;

	QGroupBox* rangeGroupBox = new QGroupBox(tr("Export time series"), this);
	_mainLayout->addWidget(rangeGroupBox);

	QGridLayout* rangeGroupLayout = new QGridLayout(rangeGroupBox);
	rangeGroupLayout->setColumnStretch(0, 5);
	rangeGroupLayout->setColumnStretch(1, 95);
	_rangeButtonGroup = new QButtonGroup(this);

	bool exportAnim = _exporter->exportAnimation();
	radioBtn = new QRadioButton(tr("Single frame"));
	_rangeButtonGroup->addButton(radioBtn, 0);
	rangeGroupLayout->addWidget(radioBtn, 0, 0, 1, 2);
	radioBtn->setChecked(!exportAnim);

	radioBtn = new QRadioButton(tr("Sequence:"));
	_rangeButtonGroup->addButton(radioBtn, 1);
	rangeGroupLayout->addWidget(radioBtn, 1, 0, 1, 2);
	radioBtn->setChecked(exportAnim);
	radioBtn->setEnabled(exporter->dataset()->animationSettings()->animationInterval().duration() != 0);

	QHBoxLayout* frameRangeLayout = new QHBoxLayout();
	rangeGroupLayout->addLayout(frameRangeLayout, 2, 1, 1, 1);

	frameRangeLayout->setSpacing(0);
	frameRangeLayout->addWidget(new QLabel(tr("From frame:")));
	_startTimeSpinner = new SpinnerWidget();
	_startTimeSpinner->setUnit(exporter->dataset()->unitsManager().timeUnit());
	_startTimeSpinner->setIntValue(exporter->dataset()->animationSettings()->frameToTime(_exporter->startFrame()));
	_startTimeSpinner->setTextBox(new QLineEdit());
	_startTimeSpinner->setMinValue(exporter->dataset()->animationSettings()->animationInterval().start());
	_startTimeSpinner->setMaxValue(exporter->dataset()->animationSettings()->animationInterval().end());
	frameRangeLayout->addWidget(_startTimeSpinner->textBox());
	frameRangeLayout->addWidget(_startTimeSpinner);
	frameRangeLayout->addSpacing(8);
	frameRangeLayout->addWidget(new QLabel(tr("To:")));
	_endTimeSpinner = new SpinnerWidget();
	_endTimeSpinner->setUnit(exporter->dataset()->unitsManager().timeUnit());
	_endTimeSpinner->setIntValue(exporter->dataset()->animationSettings()->frameToTime(_exporter->endFrame()));
	_endTimeSpinner->setTextBox(new QLineEdit());
	_endTimeSpinner->setMinValue(exporter->dataset()->animationSettings()->animationInterval().start());
	_endTimeSpinner->setMaxValue(exporter->dataset()->animationSettings()->animationInterval().end());
	frameRangeLayout->addWidget(_endTimeSpinner->textBox());
	frameRangeLayout->addWidget(_endTimeSpinner);
	frameRangeLayout->addSpacing(8);
	frameRangeLayout->addWidget(new QLabel(tr("Every Nth frame:")));
	_nthFrameSpinner = new SpinnerWidget();
	_nthFrameSpinner->setUnit(exporter->dataset()->unitsManager().integerIdentityUnit());
	_nthFrameSpinner->setIntValue(_exporter->everyNthFrame());
	_nthFrameSpinner->setTextBox(new QLineEdit());
	_nthFrameSpinner->setMinValue(1);
	frameRangeLayout->addWidget(_nthFrameSpinner->textBox());
	frameRangeLayout->addWidget(_nthFrameSpinner);

	_startTimeSpinner->setEnabled(radioBtn->isChecked());
	_endTimeSpinner->setEnabled(radioBtn->isChecked());
	_nthFrameSpinner->setEnabled(radioBtn->isChecked());
	connect(radioBtn, &QRadioButton::toggled, _startTimeSpinner, &SpinnerWidget::setEnabled);
	connect(radioBtn, &QRadioButton::toggled, _endTimeSpinner, &SpinnerWidget::setEnabled);
	connect(radioBtn, &QRadioButton::toggled, _nthFrameSpinner, &SpinnerWidget::setEnabled);

	QGroupBox* fileGroupBox = new QGroupBox(tr("Destination"), this);
	_mainLayout->addWidget(fileGroupBox);

	QGridLayout* fileGroupLayout = new QGridLayout(fileGroupBox);
	fileGroupLayout->setColumnStretch(0, 5);
	fileGroupLayout->setColumnStretch(1, 95);
	_fileGroupButtonGroup = new QButtonGroup(this);

	radioBtn = new QRadioButton(tr("Single file: %1").arg(QFileInfo(_exporter->outputFilename()).fileName()));
	_fileGroupButtonGroup->addButton(radioBtn, 0);
	fileGroupLayout->addWidget(radioBtn, 0, 0, 1, 2);
	radioBtn->setChecked(!_exporter->useWildcardFilename());

	radioBtn = new QRadioButton(tr("Multiple files (wild-card pattern):"));
	_fileGroupButtonGroup->addButton(radioBtn, 1);
	fileGroupLayout->addWidget(radioBtn, 1, 0, 1, 2);
	radioBtn->setChecked(_exporter->useWildcardFilename());

	_wildcardTextbox = new QLineEdit(_exporter->wildcardFilename(), fileGroupBox);
	fileGroupLayout->addWidget(_wildcardTextbox, 2, 1, 1, 1);
	_wildcardTextbox->setEnabled(radioBtn->isChecked());
	connect(radioBtn, &QRadioButton::toggled, _wildcardTextbox, &QLineEdit::setEnabled);

	// Show the optional UI for the exporter.
	if(PropertiesEditor::create(exporter)) {
		PropertiesPanel* propPanel = new PropertiesPanel(this, mainWindow);
		_mainLayout->addWidget(propPanel);
		propPanel->setEditObject(exporter);
	}

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &FileExporterSettingsDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &FileExporterSettingsDialog::reject);
	_mainLayout->addWidget(buttonBox);
}

/******************************************************************************
* This is called when the user has pressed the OK button.
******************************************************************************/
void FileExporterSettingsDialog::onOk()
{
	try {
		_exporter->setExportAnimation(_rangeButtonGroup->checkedId() == 1);
		_exporter->setUseWildcardFilename(_fileGroupButtonGroup->checkedId() == 1);
		_exporter->setWildcardFilename(_wildcardTextbox->text());
		_exporter->setStartFrame(_exporter->dataset()->animationSettings()->timeToFrame(_startTimeSpinner->intValue()));
		_exporter->setEndFrame(_exporter->dataset()->animationSettings()->timeToFrame(std::max(_endTimeSpinner->intValue(), _startTimeSpinner->intValue())));
		_exporter->setEveryNthFrame(_nthFrameSpinner->intValue());

		accept();
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
