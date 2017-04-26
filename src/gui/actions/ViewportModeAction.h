////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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


#include <gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(ViewportInput)

/**
 * An Qt action that activates a ViewportInputMode.
 */
class OVITO_GUI_EXPORT ViewportModeAction : public QAction
{
	Q_OBJECT

public:

	/// \brief Initializes the action object.
	ViewportModeAction(MainWindow* mainWindow, const QString& text, QObject* parent, ViewportInputMode* inputMode, const QColor& highlightColor = QColor());

	/// \brief Create a push button that activates this action.
	QPushButton* createPushButton(QWidget* parent = nullptr);

public Q_SLOTS:

	/// \brief Activates the viewport input mode.
	void activateMode() {
		onActionToggled(true);
	}

	/// \brief Deactivates the viewport input mode.
	void deactivateMode() {
		onActionToggled(false);
	}

protected Q_SLOTS:

	/// Is called when the user has triggered the action's state.
	void onActionToggled(bool checked);

private:

	/// The viewport input mode activated by this action.
	ViewportInputMode* _inputMode;

	/// The highlight color for the button controls.
	QColor _highlightColor;

	/// The viewport input manager.
	ViewportInputManager& _viewportInputManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


