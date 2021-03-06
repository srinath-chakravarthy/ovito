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

#include <core/Core.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/importexport/FileImporter.h>
#include <core/dataset/UndoStack.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/SelectionSet.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/utilities/io/ObjectSaveStream.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/ProgressDisplay.h>

#ifdef Q_OS_UNIX
	#include <signal.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

IMPLEMENT_OVITO_OBJECT(Core, DataSetContainer, RefMaker);
DEFINE_FLAGS_REFERENCE_FIELD(DataSetContainer, _currentSet, "CurrentSet", DataSet, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

/******************************************************************************
* Initializes the dataset manager.
******************************************************************************/
DataSetContainer::DataSetContainer() : RefMaker(nullptr), _taskManager()
{
	INIT_PROPERTY_FIELD(DataSetContainer::_currentSet);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void DataSetContainer::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(DataSetContainer::_currentSet)) {

		if(oldTarget) {
			DataSet* oldDataSet = static_object_cast<DataSet>(oldTarget);

			// Stop animation playback for the old dataset.
			oldDataSet->animationSettings()->stopAnimationPlayback();
		}

		// Forward signals from the current dataset.
		disconnect(_selectionSetReplacedConnection);
		disconnect(_viewportConfigReplacedConnection);
		disconnect(_animationSettingsReplacedConnection);
		disconnect(_renderSettingsReplacedConnection);
		disconnect(_filePathChangedConnection);
		disconnect(_undoStackCleanChangedConnection);
		if(currentSet()) {
			_selectionSetReplacedConnection = connect(currentSet(), &DataSet::selectionSetReplaced, this, &DataSetContainer::onSelectionSetReplaced);
			_viewportConfigReplacedConnection = connect(currentSet(), &DataSet::viewportConfigReplaced, this, &DataSetContainer::viewportConfigReplaced);
			_animationSettingsReplacedConnection = connect(currentSet(), &DataSet::animationSettingsReplaced, this, &DataSetContainer::animationSettingsReplaced);
			_renderSettingsReplacedConnection = connect(currentSet(), &DataSet::renderSettingsReplaced, this, &DataSetContainer::renderSettingsReplaced);
			_filePathChangedConnection = connect(currentSet(), &DataSet::filePathChanged, this, &DataSetContainer::filePathChanged);
			_undoStackCleanChangedConnection = connect(&currentSet()->undoStack(), &UndoStack::cleanChanged, this, &DataSetContainer::modificationStatusChanged);
		}

		Q_EMIT dataSetChanged(currentSet());

		if(currentSet()) {
			Q_EMIT viewportConfigReplaced(currentSet()->viewportConfig());
			Q_EMIT animationSettingsReplaced(currentSet()->animationSettings());
			Q_EMIT renderSettingsReplaced(currentSet()->renderSettings());
			Q_EMIT filePathChanged(currentSet()->filePath());
			Q_EMIT modificationStatusChanged(currentSet()->undoStack().isClean());
			onSelectionSetReplaced(currentSet()->selection());
			onAnimationSettingsReplaced(currentSet()->animationSettings());
		}
		else {
			onSelectionSetReplaced(nullptr);
			onAnimationSettingsReplaced(nullptr);
			Q_EMIT viewportConfigReplaced(nullptr);
			Q_EMIT animationSettingsReplaced(nullptr);
			Q_EMIT renderSettingsReplaced(nullptr);
			Q_EMIT filePathChanged(QString());
			Q_EMIT modificationStatusChanged(true);
		}
	}
	RefMaker::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* This handler is invoked when the current selection set of the current dataset
* has been replaced.
******************************************************************************/
void DataSetContainer::onSelectionSetReplaced(SelectionSet* newSelectionSet)
{
	// Forward signals from the current selection set.
	disconnect(_selectionSetChangedConnection);
	disconnect(_selectionSetChangeCompleteConnection);
	if(newSelectionSet) {
		_selectionSetChangedConnection = connect(newSelectionSet, &SelectionSet::selectionChanged, this, &DataSetContainer::selectionChanged);
		_selectionSetChangeCompleteConnection = connect(newSelectionSet, &SelectionSet::selectionChangeComplete, this, &DataSetContainer::selectionChangeComplete);
	}
	Q_EMIT selectionSetReplaced(newSelectionSet);
	Q_EMIT selectionChanged(newSelectionSet);
	Q_EMIT selectionChangeComplete(newSelectionSet);
}

/******************************************************************************
* This handler is invoked when the current animation settings of the current
* dataset have been replaced.
******************************************************************************/
void DataSetContainer::onAnimationSettingsReplaced(AnimationSettings* newAnimationSettings)
{
	// Forward signals from the current animation settings object.
	disconnect(_animationTimeChangedConnection);
	disconnect(_animationTimeChangeCompleteConnection);
	if(newAnimationSettings) {
		_animationTimeChangedConnection = connect(newAnimationSettings, &AnimationSettings::timeChanged, this, &DataSetContainer::timeChanged);
		_animationTimeChangeCompleteConnection = connect(newAnimationSettings, &AnimationSettings::timeChangeComplete, this, &DataSetContainer::timeChangeComplete);
	}
	if(newAnimationSettings) {
		Q_EMIT timeChanged(newAnimationSettings->time());
		Q_EMIT timeChangeComplete();
	}
}


/******************************************************************************
* This function blocks execution until some operation has been completed.
******************************************************************************/
bool DataSetContainer::waitUntil(const std::function<bool()>& callback, const QString& message, AbstractProgressDisplay* progressDisplay)
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "DataSetContainer::waitUntil()", "This function may only be called from the main thread.");

	// Check if operation is already completed.
	if(callback())
		return true;

	// Boolean flag which is set by the POSIX signal handler when user
	// presses Ctrl+C to interrupt the program. In console mode, the
	// DataSetContainer::waitUntil() function breaks out of the waiting loop
	// when this flag is set.
	static QAtomicInt _userInterrupt;

#ifdef Q_OS_UNIX
	// Install POSIX signal handler to catch Ctrl+C key press in console mode.
	auto oldSignalHandler = ::signal(SIGINT, [](int) { _userInterrupt.storeRelease(1); });
#endif

	try {

		// Poll callback function until it returns true.
#ifdef Q_OS_UNIX		
		while(!callback() && !_userInterrupt.loadAcquire()) {
#else
		while(!callback()) {
#endif
			QCoreApplication::processEvents();
			QThread::msleep(20);
		}

#ifdef Q_OS_UNIX
		::signal(SIGINT, oldSignalHandler);
#endif
	}
	catch(...) {
#ifdef Q_OS_UNIX
		::signal(SIGINT, oldSignalHandler);
#endif
		throw;
	}
	if(_userInterrupt.load()) {
		taskManager().cancelAll();
		return false;
	}
	return true;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
