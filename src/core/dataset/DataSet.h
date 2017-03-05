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

#pragma once


#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/animation/TimeInterval.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/Promise.h>
#include "UndoStack.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Stores the current state including the three-dimensional scene, viewport configuration,
 *        render settings etc.
 *
 * A DataSet represents the current document being edited by the user.
 * It can be completely saved to a file (.ovito extension) and loaded again at a later time.
 *
 * The DataSet class consists of various sub-objects that store different aspects. The
 * ViewportConfiguration object returned by viewportConfig(), for example, stores the list
 * of viewports.
 */
class OVITO_CORE_EXPORT DataSet : public RefTarget
{
public:

	/// \brief Constructs an empty dataset.
	/// \param self This parameter is not used and is there to provide a constructor signature that is compatible
	///             with the RefTarget base class.
	Q_INVOKABLE DataSet(DataSet* self = nullptr);

	/// \brief Destructor.
	virtual ~DataSet();

	/// \brief Returns the path where this dataset is stored on disk.
	/// \return The location where the dataset is stored or will be stored on disk.
	const QString& filePath() const { return _filePath; }

	/// \brief Sets the path where this dataset is stored.
	/// \param path The new path (should be absolute) where the dataset will be stored.
	void setFilePath(const QString& path) {
		if(path != _filePath) {
			_filePath = path;
			Q_EMIT filePathChanged(_filePath);
		}
	}

	/// \brief Returns the undo stack that keeps track of changes made to this dataset.
	UndoStack& undoStack() {
		OVITO_CHECK_OBJECT_POINTER(this);
		return _undoStack;
	}

	/// \brief Returns the manager of ParameterUnit objects.
	UnitsManager& unitsManager() { return _unitsManager; }

	/// \brief Returns a pointer to the main window in which this dataset is being edited.
	/// \return The main window, or NULL if this data set is not being edited in any window.
	//MainWindow* mainWindow() const;

	/// \brief Returns the container this dataset belongs to.
	DataSetContainer* container() const;

	/// \brief Deletes all nodes from the scene.
	/// \undoable
	Q_INVOKABLE void clearScene();

	/// \brief Rescales the animation keys of all controllers in the scene.
	/// \param oldAnimationInterval The old animation interval, which will be mapped to the new animation interval.
	/// \param newAnimationInterval The new animation interval.
	///
	/// This method calls Controller::rescaleTime() for all controllers in the scene.
	/// For keyed controllers this will rescale the key times of all keys from the
	/// old animation interval to the new interval using a linear mapping.
	///
	/// Keys that lie outside of the old active animation interval will also be rescaled
	/// according to a linear extrapolation.
	///
	/// \undoable
	void rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval);

	/// \brief This is the high-level rendering function, which invokes the renderer to generate one or more
	///        output images of the scene. All rendering parameters are specified in the RenderSettings object.
	/// \param settings A RenderSettings object that specifies output image size, animation range to render etc.
	/// \param viewport The viewport to render. This determines the camera orientation.
	/// \param frameBuffer The frame buffer that will receive the rendered image. When rendering an animation
	///        sequence, the buffer will contain only the last rendered frame when the function returns.
	/// \return true on success; false if operation has been canceled by the user.
	/// \throw Exception on error.
	bool renderScene(RenderSettings* settings, Viewport* viewport, FrameBuffer* frameBuffer, TaskManager& taskManager);

	/// \brief This function returns a future that is triggered once all data pipelines in the scene become ready.
	/// \param message An optional messge text to be shown to the user while waiting.
	Future<void> makeSceneReady(const QString& message = QString());

	/// \brief Saves the dataset to the given file.
	/// \throw Exception on error.
	///
	/// Note that this method does NOT invoke setFilePath().
	void saveToFile(const QString& filePath);

	/// \brief Appends an object to this dataset's list of global objects.
	void addGlobalObject(RefTarget* target) {
		if(!_globalObjects.contains(target))
			_globalObjects.push_back(target);
	}

	/// \brief Removes an object from this dataset's list of global objects.
	void removeGlobalObject(int index) {
		_globalObjects.remove(index);
	}

	/// \brief Looks for a global object of the given type.
	template<class T>
	T* findGlobalObject() const {
		for(RefTarget* obj : globalObjects()) {
			T* castObj = dynamic_object_cast<T>(obj);
			if(castObj) return castObj;
		}
		return nullptr;
	}	

Q_SIGNALS:

	/// \brief This signal is emitted whenever the current viewport configuration of this dataset
	///        has been replaced by a new one.
	/// \note This signal is NOT emitted when parameters of the current viewport configuration change.
    void viewportConfigReplaced(ViewportConfiguration* newViewportConfiguration);

	/// \brief This signal is emitted whenever the current animation settings of this dataset
	///        have been replaced by new ones.
	/// \note This signal is NOT emitted when parameters of the current animation settings object change.
    void animationSettingsReplaced(AnimationSettings* newAnimationSettings);

	/// \brief This signal is emitted whenever the current render settings of this dataset
	///        have been replaced by new ones.
	/// \note This signal is NOT emitted when parameters of the current render settings object change.
    void renderSettingsReplaced(RenderSettings* newRenderSettings);

	/// \brief This signal is emitted whenever the current selection set of this dataset
	///        has been replaced by another one.
	/// \note This signal is NOT emitted when nodes are added or removed from the current selection set.
    void selectionSetReplaced(SelectionSet* newSelectionSet);

	/// \brief This signal is emitted whenever the dataset has been saved under a new file name.
    void filePathChanged(const QString& filePath);

protected:

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// Renders a single frame and saves the output file. This is part of the implementation of the renderScene() method.
	bool renderFrame(TimePoint renderTime, int frameNumber, RenderSettings* settings, SceneRenderer* renderer,
			Viewport* viewport, FrameBuffer* frameBuffer, VideoEncoder* videoEncoder, TaskManager& taskManager);

	/// Returns a viewport configuration that is used as template for new scenes.
	OORef<ViewportConfiguration> createDefaultViewportConfiguration();

	/// \brief Checks if all scene nodes are ready and their data pipelines can provide fully computed results.
	bool isSceneReady(TimePoint time) const;

private:

	/// The configuration of the viewports.
	DECLARE_REFERENCE_FIELD(ViewportConfiguration, viewportConfig);

	/// Current animation settings.
	DECLARE_REFERENCE_FIELD(AnimationSettings, animationSettings);

	/// Root node of the scene node tree.
	DECLARE_REFERENCE_FIELD(SceneRoot, sceneRoot);

	/// The current node selection set.
	DECLARE_REFERENCE_FIELD(SelectionSet, selection);

	/// The settings used when rendering the scene.
	DECLARE_REFERENCE_FIELD(RenderSettings, renderSettings);

	/// Global data managed by plugins.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(RefTarget, globalObjects, setGlobalObjects);

	/// The file path this DataSet has been saved to.
	QString _filePath;

	/// The undo stack that keeps track of changes made to this dataset.
	UndoStack _undoStack;

	/// The manager of ParameterUnit objects.
	UnitsManager _unitsManager;

	/// Active request waiting for the scene to become ready.
	PromisePtr<void> _sceneReadyRequest;

	/// This signal/slot connection updates the viewports when the animation time changes.
	QMetaObject::Connection _updateViewportOnTimeChangeConnection;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


