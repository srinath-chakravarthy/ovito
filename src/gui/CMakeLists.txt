###############################################################################
# 
#  Copyright (2016) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

SET(SourceFiles 
	mainwin/MainWindow.cpp
	mainwin/ViewportsPanel.cpp
	mainwin/TaskDisplayWidget.cpp
	mainwin/cmdpanel/CommandPanel.cpp
	mainwin/cmdpanel/RenderCommandPage.cpp
	mainwin/cmdpanel/ModifyCommandPage.cpp
	mainwin/cmdpanel/ModificationListModel.cpp
	mainwin/cmdpanel/ModificationListItem.cpp
	mainwin/cmdpanel/ModifierListBox.cpp
	mainwin/cmdpanel/UtilityCommandPage.cpp
	mainwin/cmdpanel/OverlayCommandPage.cpp
	dialogs/AdjustCameraDialog.cpp
	dialogs/AnimationKeyEditorDialog.cpp
	dialogs/AnimationSettingsDialog.cpp
	dialogs/ApplicationSettingsDialog.cpp
	dialogs/GeneralSettingsPage.cpp
	dialogs/ViewportSettingsPage.cpp
	dialogs/HistoryFileDialog.cpp
	dialogs/ImportFileDialog.cpp
	dialogs/ImportRemoteFileDialog.cpp
	dialogs/SaveImageFileDialog.cpp
	dialogs/LoadImageFileDialog.cpp
	dialogs/RemoteAuthenticationDialog.cpp
	dialogs/FileExporterSettingsDialog.cpp
	actions/ActionManager.cpp
	actions/FileActions.cpp
	actions/ViewportActions.cpp
	actions/AnimationActions.cpp
	actions/RenderActions.cpp
	actions/ViewportModeAction.cpp
	dataset/importexport/FileSourceEditor.cpp
	dataset/importexport/FileImporterEditor.cpp
	dataset/importexport/AttributeFileExporterEditor.cpp
	dataset/GuiDataSetContainer.cpp
	widgets/general/RolloutContainer.cpp
	widgets/general/SpinnerWidget.cpp
	widgets/general/ColorPickerWidget.cpp
	widgets/general/ElidedTextLabel.cpp
	widgets/general/AutocompleteLineEdit.cpp
	widgets/general/AutocompleteTextEdit.cpp
	widgets/general/HtmlListWidget.cpp
	widgets/animation/AnimationTimeSpinner.cpp
	widgets/animation/AnimationFramesToolButton.cpp
	widgets/animation/AnimationTimeSlider.cpp
	widgets/animation/AnimationTrackBar.cpp
	widgets/rendering/FrameBufferWidget.cpp
	widgets/rendering/FrameBufferWindow.cpp
	widgets/selection/SceneNodesListModel.cpp
	widgets/selection/SceneNodeSelectionBox.cpp
	widgets/display/CoordinateDisplayWidget.cpp
	widgets/display/StatusWidget.cpp
	plugins/utility/UtilityApplet.cpp
	plugins/autostart/GuiAutoStartObject.cpp
	properties/PropertiesEditor.cpp
	properties/PropertiesPanel.cpp
	properties/ParameterUI.cpp
	properties/NumericalParameterUI.cpp
	properties/AffineTransformationParameterUI.cpp
	properties/BooleanActionParameterUI.cpp
	properties/BooleanGroupBoxParameterUI.cpp
	properties/BooleanParameterUI.cpp
	properties/BooleanRadioButtonParameterUI.cpp
	properties/ColorParameterUI.cpp
	properties/CustomParameterUI.cpp
	properties/FilenameParameterUI.cpp
	properties/FloatParameterUI.cpp
	properties/FontParameterUI.cpp
	properties/IntegerParameterUI.cpp
	properties/IntegerRadioButtonParameterUI.cpp
	properties/RefTargetListParameterUI.cpp
	properties/StringParameterUI.cpp
	properties/SubObjectParameterUI.cpp
	properties/VariantComboBoxParameterUI.cpp
	properties/Vector3ParameterUI.cpp
	rendering/ViewportSceneRenderer.cpp
	rendering/RenderSettingsEditor.cpp
	viewport/ViewportWindow.cpp
	viewport/ViewportMenu.cpp
	viewport/input/ViewportInputManager.cpp
	viewport/input/ViewportInputMode.cpp
	viewport/input/NavigationModes.cpp
	viewport/input/XFormModes.cpp
	viewport/picking/PickingSceneRenderer.cpp
	viewport/overlay/CoordinateTripodOverlayEditor.cpp
	viewport/overlay/TextLabelOverlayEditor.cpp
	viewport/overlay/MoveOverlayInputMode.cpp
	app/GuiApplication.cpp
	scene/objects/camera/CameraObjectEditor.cpp
	scene/objects/geometry/TriMeshDisplayEditor.cpp
	utilities/io/GuiFileManager.cpp
	utilities/concurrent/ProgressDialog.cpp
	animation/controller/TCBInterpolationControllerEditor.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/gui.qrc)

# Build library
ADD_LIBRARY(Gui SHARED GUI.cpp ${SourceFiles} ${ResourceFiles})

# Pass name of gui plugin to the code.
TARGET_COMPILE_DEFINITIONS(Gui PRIVATE "OVITO_PLUGIN_NAME=\"Gui\"")

# Set default include directory.
TARGET_INCLUDE_DIRECTORIES(Gui PUBLIC "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

# GUI is based on Core and the OpenGL rendering library.
TARGET_LINK_LIBRARIES(Gui PUBLIC Core OpenGLRenderer)

# Link Qt5.
TARGET_LINK_LIBRARIES(Gui PUBLIC Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(Gui gui/GUI.h)

IF(APPLE)
    # Activate @rpath on MacOS.
    SET_TARGET_PROPERTIES(Gui PROPERTIES MACOSX_RPATH TRUE)
	# This is required to avoid error by install_name_tool on MacOS.
	SET_TARGET_PROPERTIES(Gui PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(Gui PROPERTIES INSTALL_RPATH "$ORIGIN")
ENDIF()
    
# This library is part of the installation package.
INSTALL(TARGETS Gui EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

# Export target to make it accessible for external plugins.
IF(CMAKE_VERSION VERSION_LESS "3")
	EXPORT(TARGETS Gui NAMESPACE "Ovito::" APPEND FILE "${${PROJECT_NAME}_BINARY_DIR}/OVITOTargets.cmake")
ENDIF()
