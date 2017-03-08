///////////////////////////////////////////////////////////////////////////////
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

#include <gui/GUI.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/viewport/ViewportWindowInterface.h>
#include <core/app/Application.h>
#include <core/app/StandaloneApplication.h>
#include <gui/actions/ActionManager.h>
#include <gui/widgets/animation/AnimationTimeSpinner.h>
#include <gui/widgets/animation/AnimationFramesToolButton.h>
#include <gui/widgets/animation/AnimationTimeSlider.h>
#include <gui/widgets/animation/AnimationTrackBar.h>
#include <gui/widgets/rendering/FrameBufferWindow.h>
#include <gui/widgets/display/CoordinateDisplayWidget.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/plugins/autostart/GuiAutoStartObject.h>
#include <opengl_renderer/OpenGLSceneRenderer.h>
#include "MainWindow.h"
#include "ViewportsPanel.h"
#include "TaskDisplayWidget.h"
#include "cmdpanel/CommandPanel.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/******************************************************************************
* The constructor of the main window class.
******************************************************************************/
MainWindow::MainWindow() : _datasetContainer(this)
{
	setWindowTitle(tr("Ovito (Open Visualization Tool)"));
	setAttribute(Qt::WA_DeleteOnClose);

	// Setup the layout of docking widgets.
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

	// Disable context menus in toolbars.
	setContextMenuPolicy(Qt::NoContextMenu);

	// Create input manager.
	_viewportInputManager = new ViewportInputManager(this);

	// Create actions.
	_actionManager = new ActionManager(this);

	// Let GUI auto-start objects register their actions.
	for(const auto& obj : Application::instance()->autostartObjects()) {
		if(auto gui_obj = dynamic_object_cast<GuiAutoStartObject>(obj))
			gui_obj->registerActions(*_actionManager);
	}

	// Create the main menu
	createMainMenu();

	// Create the main toolbar.
	createMainToolbar();

	_viewportsPanel = new ViewportsPanel(this);
	setCentralWidget(_viewportsPanel);

	// Create the animation panel below the viewports.
	QWidget* animationPanel = new QWidget();
	QVBoxLayout* animationPanelLayout = new QVBoxLayout();
	animationPanelLayout->setSpacing(0);
	animationPanelLayout->setContentsMargins(0, 1, 0, 0);
	animationPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
#if defined(Q_OS_LINUX)
	QHBoxLayout* animationPanelParentLayout = new QHBoxLayout(animationPanel);
	animationPanelParentLayout->setSpacing(0);
	animationPanelParentLayout->setContentsMargins(0, 0, 0, 0);
	animationPanelParentLayout->addLayout(animationPanelLayout, 1);
	QFrame* verticalRule = new QFrame(animationPanel);
	verticalRule->setContentsMargins(0,0,0,0);
	verticalRule->setFrameStyle(QFrame::VLine | QFrame::Sunken);
	verticalRule->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	animationPanelParentLayout->addWidget(verticalRule);
#else
	animationPanel->setLayout(animationPanelLayout);
#endif

	// Create animation time slider
	AnimationTimeSlider* timeSlider = new AnimationTimeSlider(this);
	animationPanelLayout->addWidget(timeSlider);
	AnimationTrackBar* trackBar = new AnimationTrackBar(this, timeSlider);
	animationPanelLayout->addWidget(trackBar);

	// Create status bar.
	_statusBarLayout = new QHBoxLayout();
	_statusBarLayout->setContentsMargins(0,0,0,0);
	_statusBarLayout->setSpacing(0);
	animationPanelLayout->addLayout(_statusBarLayout, 1);

	_statusBar = new QStatusBar(animationPanel);
	_statusBar->setSizeGripEnabled(false);
	_statusBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	setStatusBar(_statusBar);
	_statusBarLayout->addWidget(_statusBar, 1);

	TaskDisplayWidget* taskDisplay = new TaskDisplayWidget(this);
	_statusBarLayout->insertWidget(1, taskDisplay);

	_coordinateDisplay = new CoordinateDisplayWidget(datasetContainer(), animationPanel);
	_statusBarLayout->addWidget(_coordinateDisplay);

	// Create the animation control toolbar.
	QToolBar* animationControlBar1 = new QToolBar();
	animationControlBar1->addAction(actionManager()->getAction(ACTION_GOTO_START_OF_ANIMATION));
	animationControlBar1->addSeparator();
	animationControlBar1->addAction(actionManager()->getAction(ACTION_GOTO_PREVIOUS_FRAME));
	animationControlBar1->addAction(actionManager()->getAction(ACTION_TOGGLE_ANIMATION_PLAYBACK));
	animationControlBar1->addAction(actionManager()->getAction(ACTION_GOTO_NEXT_FRAME));
	animationControlBar1->addSeparator();
	animationControlBar1->addAction(actionManager()->getAction(ACTION_GOTO_END_OF_ANIMATION));
	QToolBar* animationControlBar2 = new QToolBar();
#if 1
	animationControlBar2->addAction(actionManager()->getAction(ACTION_AUTO_KEY_MODE_TOGGLE));
#else
	animationControlBar2->addWidget(new AnimationFramesToolButton(datasetContainer()));
#endif
	QWidget* animationTimeSpinnerContainer = new QWidget();
	QHBoxLayout* animationTimeSpinnerLayout = new QHBoxLayout(animationTimeSpinnerContainer);
	animationTimeSpinnerLayout->setContentsMargins(0,0,0,0);
	animationTimeSpinnerLayout->setSpacing(0);
	class TimeEditBox : public QLineEdit {
	public:
		virtual QSize sizeHint() const { return minimumSizeHint(); }
	};
	QLineEdit* timeEditBox = new TimeEditBox();
	timeEditBox->setToolTip(tr("Current Animation Time"));
	AnimationTimeSpinner* currentTimeSpinner = new AnimationTimeSpinner(this);
	currentTimeSpinner->setTextBox(timeEditBox);
	animationTimeSpinnerLayout->addWidget(timeEditBox, 1);
	animationTimeSpinnerLayout->addWidget(currentTimeSpinner);
	animationControlBar2->addWidget(animationTimeSpinnerContainer);
	animationControlBar2->addAction(actionManager()->getAction(ACTION_ANIMATION_SETTINGS));
	animationControlBar2->addWidget(new QWidget());

	QWidget* animationControlPanel = new QWidget();
	QVBoxLayout* animationControlPanelLayout = new QVBoxLayout(animationControlPanel);
	animationControlPanelLayout->setSpacing(0);
	animationControlPanelLayout->setContentsMargins(0, 1, 0, 0);
	animationControlPanelLayout->addWidget(animationControlBar1);
	animationControlPanelLayout->addWidget(animationControlBar2);
	animationControlPanelLayout->addStretch(1);
	animationControlPanel->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; } QToolButton { padding: 0px; margin: 0px }");
	animationControlPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

	// Create the viewport control toolbar.
	QToolBar* viewportControlBar1 = new QToolBar();
	viewportControlBar1->addAction(actionManager()->getAction(ACTION_VIEWPORT_ZOOM));
	viewportControlBar1->addAction(actionManager()->getAction(ACTION_VIEWPORT_PAN));
	viewportControlBar1->addAction(actionManager()->getAction(ACTION_VIEWPORT_ORBIT));
	viewportControlBar1->addAction(actionManager()->getAction(ACTION_VIEWPORT_PICK_ORBIT_CENTER));
	QToolBar* viewportControlBar2 = new QToolBar();
	viewportControlBar2->addAction(actionManager()->getAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS));
	viewportControlBar2->addAction(actionManager()->getAction(ACTION_VIEWPORT_ZOOM_SCENE_EXTENTS_ALL));
	viewportControlBar2->addAction(actionManager()->getAction(ACTION_VIEWPORT_FOV));
	viewportControlBar2->addAction(actionManager()->getAction(ACTION_VIEWPORT_MAXIMIZE));
	QWidget* viewportControlPanel = new QWidget();
	QVBoxLayout* viewportControlPanelLayout = new QVBoxLayout(viewportControlPanel);
	viewportControlPanelLayout->setSpacing(0);
	viewportControlPanelLayout->setContentsMargins(0, 1, 0, 0);
	viewportControlPanelLayout->addWidget(viewportControlBar1);
	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->addStretch(1);
	sublayout->addWidget(viewportControlBar2);
	viewportControlPanelLayout->addLayout(sublayout);
	viewportControlPanelLayout->addStretch(1);
	viewportControlPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	viewportControlPanel->setStyleSheet("QToolBar { padding: 0px; margin: 0px; border: 0px none black; } QToolButton { padding: 0px; margin: 0px }");

	// Create the command panel.
	_commandPanel = new CommandPanel(this, this);

	createDockPanel(tr("Animation Panel"), "AnimationPanel", Qt::BottomDockWidgetArea, Qt::BottomDockWidgetArea, animationPanel);
	createDockPanel(tr("Animation Control Panel"), "AnimationControlPanel", Qt::BottomDockWidgetArea, Qt::BottomDockWidgetArea, animationControlPanel);
	createDockPanel(tr("Viewport Control"), "ViewportControlPanel", Qt::BottomDockWidgetArea, Qt::BottomDockWidgetArea, viewportControlPanel);
	createDockPanel(tr("Command Panel"), "CommandPanel", Qt::RightDockWidgetArea, Qt::DockWidgetAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea), _commandPanel);

	// Create the frame buffer window.
	_frameBufferWindow = new FrameBufferWindow(this);

	// Update window title when document path changes.
	connect(&_datasetContainer, &DataSetContainer::filePathChanged, [this](const QString& filePath) { setWindowFilePath(filePath); });
	connect(&_datasetContainer, &DataSetContainer::modificationStatusChanged, [this](bool isClean) { setWindowModified(!isClean); });
}

/******************************************************************************
* Destructor.
******************************************************************************/
MainWindow::~MainWindow()
{
}

/******************************************************************************
* Returns the main window in which the given dataset is opened.
******************************************************************************/
MainWindow* MainWindow::fromDataset(DataSet* dataset)
{
	if(GuiDataSetContainer* container = qobject_cast<GuiDataSetContainer*>(dataset->container()))
		return container->mainWindow();
	return nullptr;
}

/******************************************************************************
* Creates a dock panel.
******************************************************************************/
void MainWindow::createDockPanel(const QString& caption, const QString& objectName, Qt::DockWidgetArea dockArea, Qt::DockWidgetAreas allowedAreas, QWidget* contents)
{
	QDockWidget* dockWidget = new QDockWidget(caption, this);
	dockWidget->setObjectName(objectName);
	dockWidget->setAllowedAreas(allowedAreas);
	dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
	dockWidget->setWidget(contents);
	dockWidget->setTitleBarWidget(new QWidget());
	addDockWidget(dockArea, dockWidget);
}

/******************************************************************************
* Loads the layout of the docked widgets from the settings store.
******************************************************************************/
void MainWindow::restoreLayout()
{
	QSettings settings;
	settings.beginGroup("app/mainwindow");
	QVariant state = settings.value("state");
	if(state.canConvert<QByteArray>())
		restoreState(state.toByteArray());
}

/******************************************************************************
* Saves the layout of the docked widgets to the settings store.
******************************************************************************/
void MainWindow::saveLayout()
{
	QSettings settings;
	settings.beginGroup("app/mainwindow");
	settings.setValue("state", saveState());
}

/******************************************************************************
* Creates the main menu.
******************************************************************************/
void MainWindow::createMainMenu()
{
	QMenuBar* menuBar = new QMenuBar(this);

	// Build the file menu.
	QMenu* fileMenu = menuBar->addMenu(tr("&File"));
	fileMenu->setObjectName(QStringLiteral("FileMenu"));
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_IMPORT));
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_REMOTE_IMPORT));
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_EXPORT));
	fileMenu->addSeparator();
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_OPEN));
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_SAVE));
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_SAVEAS));
	fileMenu->addSeparator();
	fileMenu->addAction(actionManager()->getAction(ACTION_FILE_NEW_WINDOW));
	fileMenu->addSeparator();
	fileMenu->addAction(actionManager()->getAction(ACTION_QUIT));

	// Build the edit menu.
	QMenu* editMenu = menuBar->addMenu(tr("&Edit"));
	editMenu->setObjectName(QStringLiteral("EditMenu"));
	editMenu->addAction(actionManager()->getAction(ACTION_EDIT_UNDO));
	editMenu->addAction(actionManager()->getAction(ACTION_EDIT_REDO));
	editMenu->addSeparator();
	editMenu->addAction(actionManager()->getAction(ACTION_EDIT_DELETE));

	// Build the scripting menu.
	QAction* runScriptFileAction = actionManager()->findAction(ACTION_SCRIPTING_RUN_FILE);
	if(runScriptFileAction) {
		QMenu* scriptingMenu = menuBar->addMenu(tr("&Scripting"));
		scriptingMenu->setObjectName(QStringLiteral("ScriptingMenu"));
		scriptingMenu->addAction(runScriptFileAction);
	}

	// Build the options menu.
	QMenu* optionsMenu = menuBar->addMenu(tr("&Options"));
	optionsMenu->setObjectName(QStringLiteral("OptionsMenu"));
	optionsMenu->addAction(actionManager()->getAction(ACTION_SETTINGS_DIALOG));

	// Build the help menu.
	QMenu* helpMenu = menuBar->addMenu(tr("&Help"));
	helpMenu->setObjectName(QStringLiteral("HelpMenu"));
	helpMenu->addAction(actionManager()->getAction(ACTION_HELP_SHOW_ONLINE_HELP));
	connect(helpMenu->addAction(tr("Scripting Reference")), &QAction::triggered, [this]() {
		openHelpTopic(QStringLiteral("python/index.html"));
	});
	helpMenu->addSeparator();
	helpMenu->addAction(actionManager()->getAction(ACTION_HELP_OPENGL_INFO));
#ifndef  Q_OS_MACX
	helpMenu->addSeparator();
#endif
	helpMenu->addAction(actionManager()->getAction(ACTION_HELP_ABOUT));

	// Let GUI auto-start objects add their actions to the main menu.
	for(const auto& obj : StandaloneApplication::instance()->autostartObjects()) {
		if(auto gui_obj = dynamic_object_cast<GuiAutoStartObject>(obj))
			gui_obj->addActionsToMenu(*_actionManager, menuBar);
	}

	setMenuBar(menuBar);
}

/******************************************************************************
* Creates the main toolbar.
******************************************************************************/
void MainWindow::createMainToolbar()
{
	_mainToolbar = addToolBar(tr("Main Toolbar"));
	_mainToolbar->setObjectName("MainToolbar");

	_mainToolbar->addAction(actionManager()->getAction(ACTION_FILE_IMPORT));
	_mainToolbar->addAction(actionManager()->getAction(ACTION_FILE_REMOTE_IMPORT));

	_mainToolbar->addSeparator();

	_mainToolbar->addAction(actionManager()->getAction(ACTION_FILE_OPEN));
	_mainToolbar->addAction(actionManager()->getAction(ACTION_FILE_SAVE));

	_mainToolbar->addSeparator();

	_mainToolbar->addAction(actionManager()->getAction(ACTION_EDIT_UNDO));
	_mainToolbar->addAction(actionManager()->getAction(ACTION_EDIT_REDO));

#if 1
	_mainToolbar->addSeparator();

	_mainToolbar->addAction(actionManager()->getAction(ACTION_SELECTION_MODE));
	_mainToolbar->addAction(actionManager()->getAction(ACTION_XFORM_MOVE_MODE));
	_mainToolbar->addAction(actionManager()->getAction(ACTION_XFORM_ROTATE_MODE));
#endif

	_mainToolbar->addSeparator();

	_mainToolbar->addAction(actionManager()->getAction(ACTION_RENDER_ACTIVE_VIEWPORT));
}

/******************************************************************************
* Is called when the window receives an event.
******************************************************************************/
bool MainWindow::event(QEvent* event)
{
	if(event->type() == QEvent::StatusTip) {
		statusBar()->showMessage(static_cast<QStatusTipEvent*>(event)->tip());
		return true;
	}
	return QMainWindow::event(event);
}

/******************************************************************************
* Is called when the user closes the window.
******************************************************************************/
void MainWindow::closeEvent(QCloseEvent* event)
{
	try {
		// Save changes.
		if(!datasetContainer().askForSaveChanges()) {
			event->ignore();
			return;
		}

		// Save window layout.
		saveLayout();

		// Destroy main window.
		event->accept();
	}
	catch(const Exception& ex) {
		event->ignore();
		ex.reportError();
	}
}

/******************************************************************************
* Immediately repaints all viewports that are flagged for an update.
******************************************************************************/
void MainWindow::processViewportUpdates()
{
	datasetContainer().currentSet()->viewportConfig()->processViewportUpdates();
}

/******************************************************************************
* Shows the online manual and opens the given help page.
******************************************************************************/
void MainWindow::openHelpTopic(const QString& page)
{
	QDir prefixDir(QCoreApplication::applicationDirPath());
#if defined(Q_OS_WIN)
	QDir helpDir = QDir(prefixDir.absolutePath() + "/doc/manual/html/");
#elif defined(Q_OS_MAC)
	prefixDir.cdUp();
	QDir helpDir = QDir(prefixDir.absolutePath() + "/Resources/doc/manual/html/");
#else
	prefixDir.cdUp();
	QDir helpDir = QDir(prefixDir.absolutePath() + "/share/ovito/doc/manual/html/");
#endif

	// Use the web browser to display online help.
	QString fullPath = helpDir.absoluteFilePath(page.isEmpty() ? QStringLiteral("index.html") : page);
	if(!QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath))) {
		Exception(tr("Could not launch web browser to display online manual. The requested file path is %1").arg(fullPath)).reportError();
	}
}

/******************************************************************************
* Returns the master OpenGL context managed by this window, which is used to
* render the viewports. If sharing of OpenGL contexts between viewports is
* disabled, then this function returns the GL context of the first viewport
* in this window.
******************************************************************************/
QOpenGLContext* MainWindow::getOpenGLContext()
{
	if(_glcontext)
		return _glcontext;

	if(OpenGLSceneRenderer::contextSharingEnabled()) {
		_glcontext = new QOpenGLContext(this);
		_glcontext->setFormat(ViewportSceneRenderer::getDefaultSurfaceFormat());
		if(!_glcontext->create())
			throw Exception(tr("Failed to create OpenGL context."), &datasetContainer());
	}
	else {
		ViewportWindow* vpWindow = viewportsPanel()->findChild<ViewportWindow*>();
		if(vpWindow)
			_glcontext = vpWindow->context();
	}

	return _glcontext;
}

/******************************************************************************
* Returns the page of the command panel that is currently visible.
******************************************************************************/
MainWindow::CommandPanelPage MainWindow::currentCommandPanelPage() const
{
	return _commandPanel->currentPage();
}

/******************************************************************************
* Sets the page of the command panel that is currently visible.
******************************************************************************/
void MainWindow::setCurrentCommandPanelPage(CommandPanelPage page)
{
	_commandPanel->setCurrentPage(page);
}

/******************************************************************************
* Sets the file path associated with this window and updates the window's title.
******************************************************************************/
void MainWindow::setWindowFilePath(const QString& filePath)
{
	if(filePath.isEmpty())
		setWindowTitle(tr("Ovito (Open Visualization Tool) [*]"));
	else
		setWindowTitle(tr("Ovito (Open Visualization Tool) - %1[*]").arg(QFileInfo(filePath).fileName()));
	QMainWindow::setWindowFilePath(filePath);
}


OVITO_END_INLINE_NAMESPACE
}	// End of namespace
