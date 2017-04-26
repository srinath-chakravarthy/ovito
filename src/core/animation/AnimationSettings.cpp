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
#include "AnimationSettings.h"

#include <QTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AnimationSettings, RefTarget);
DEFINE_FLAGS_PROPERTY_FIELD(AnimationSettings, time, "Time", PROPERTY_FIELD_NO_UNDO);
DEFINE_PROPERTY_FIELD(AnimationSettings, animationInterval, "AnimationInterval");
DEFINE_PROPERTY_FIELD(AnimationSettings, ticksPerFrame, "TicksPerFrame");
DEFINE_PROPERTY_FIELD(AnimationSettings, playbackSpeed, "PlaybackSpeed");
DEFINE_PROPERTY_FIELD(AnimationSettings, loopPlayback, "LoopPlayback");

/******************************************************************************
* Constructor.
******************************************************************************/
AnimationSettings::AnimationSettings(DataSet* dataset) : RefTarget(dataset),
		_ticksPerFrame(TICKS_PER_SECOND/10), _playbackSpeed(1),
		_animationInterval(0, 0), _time(0),
		_loopPlayback(true)
{
	INIT_PROPERTY_FIELD(time);
	INIT_PROPERTY_FIELD(animationInterval);
	INIT_PROPERTY_FIELD(ticksPerFrame);
	INIT_PROPERTY_FIELD(playbackSpeed);
	INIT_PROPERTY_FIELD(loopPlayback);

	// Call our own listener when the current animation time changes.
	connect(this, &AnimationSettings::timeChanged, this, &AnimationSettings::onTimeChanged);
}

/******************************************************************************
* Is called when the value of a non-animatable property field of this RefMaker has changed.
******************************************************************************/
void AnimationSettings::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(time))
		Q_EMIT timeChanged(time());
	else if(field == PROPERTY_FIELD(animationInterval))
		Q_EMIT intervalChanged(animationInterval());
	else if(field == PROPERTY_FIELD(ticksPerFrame))
		Q_EMIT speedChanged(ticksPerFrame());
}

/******************************************************************************
* Saves the class' contents to an output stream.
******************************************************************************/
void AnimationSettings::saveToStream(ObjectSaveStream& stream)
{
	RefTarget::saveToStream(stream);
	stream.beginChunk(0x01);
	stream << _namedFrames;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from an input stream.
******************************************************************************/
void AnimationSettings::loadFromStream(ObjectLoadStream& stream)
{
	RefTarget::loadFromStream(stream);
	stream.expectChunk(0x01);
	stream >> _namedFrames;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> AnimationSettings::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<AnimationSettings> clone = static_object_cast<AnimationSettings>(RefTarget::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_namedFrames = this->_namedFrames;

	return clone;
}

/******************************************************************************
* Is called when the current animation time has changed.
******************************************************************************/
void AnimationSettings::onTimeChanged(TimePoint newTime)
{
	if(_isTimeChanging) {
		dataset()->makeSceneReady();
		return;
	}
	_isTimeChanging = true;

	// Wait until scene is complete, then generate a timeChangeComplete event.
	PromiseWatcher* watcher = new PromiseWatcher(this);
	connect(watcher, &PromiseWatcher::finished, this, [this] {
		_isTimeChanging = false;
		Q_EMIT timeChangeComplete();
	});
	connect(watcher, &PromiseWatcher::finished, watcher, &QObject::deleteLater);	// Self-destruct watcher object when it's no longer needed.
	watcher->setFuture(dataset()->makeSceneReady());
}

/******************************************************************************
* Converts a time value to its string representation.
******************************************************************************/
QString AnimationSettings::timeToString(TimePoint time)
{
	return QString::number(timeToFrame(time));
}

/******************************************************************************
* Converts a string to a time value.
* Throws an exception when a parsing error occurs.
******************************************************************************/
TimePoint AnimationSettings::stringToTime(const QString& stringValue)
{
	TimePoint value;
	bool ok;
	value = (TimePoint)stringValue.toInt(&ok);
	if(!ok)
		throwException(tr("Invalid frame number format: %1").arg(stringValue));
	return frameToTime(value);
}

/******************************************************************************
* Enables or disables auto key generation mode.
******************************************************************************/
void AnimationSettings::setAutoKeyMode(bool on)
{
	if(_autoKeyMode == on)
		return;

	_autoKeyMode = on;
	Q_EMIT autoKeyModeChanged(_autoKeyMode);
}

/******************************************************************************
* Sets the current animation time to the start of the animation interval.
******************************************************************************/
void AnimationSettings::jumpToAnimationStart()
{
	setTime(animationInterval().start());
}

/******************************************************************************
* Sets the current animation time to the end of the animation interval.
******************************************************************************/
void AnimationSettings::jumpToAnimationEnd()
{
	setTime(animationInterval().end());
}

/******************************************************************************
* Jumps to the previous animation frame.
******************************************************************************/
void AnimationSettings::jumpToPreviousFrame()
{
	// Subtract one frame from current time.
	TimePoint newTime = frameToTime(timeToFrame(time()) - 1);
	// Clamp new time
	newTime = std::max(newTime, animationInterval().start());
	// Set new time.
	setTime(newTime);
}

/******************************************************************************
* Jumps to the previous animation frame.
******************************************************************************/
void AnimationSettings::jumpToNextFrame()
{
	// Subtract one frame from current time.
	TimePoint newTime = frameToTime(timeToFrame(time()) + 1);
	// Clamp new time
	newTime = std::min(newTime, animationInterval().end());
	// Set new time.
	setTime(newTime);
}

/******************************************************************************
* Starts playback of the animation in the viewports.
******************************************************************************/
void AnimationSettings::startAnimationPlayback()
{
	if(!isPlaybackActive()) {
		_isPlaybackActive = true;
		Q_EMIT playbackChanged(_isPlaybackActive);

		if(time() < animationInterval().end()) {
			scheduleNextAnimationFrame();
		}
		else {
			continuePlaybackAtTime(animationInterval().start());
		}
	}
}

/******************************************************************************
* Jumps to the given animation time, then schedules the next frame as soon as 
* the scene was completely shown.
******************************************************************************/
void AnimationSettings::continuePlaybackAtTime(TimePoint time)
{
	setTime(time);
	
	if(isPlaybackActive()) {
		PromiseWatcher* watcher = new PromiseWatcher(this);
		connect(watcher, &PromiseWatcher::finished, this, &AnimationSettings::scheduleNextAnimationFrame);
		connect(watcher, &PromiseWatcher::canceled, this, &AnimationSettings::stopAnimationPlayback);
		connect(watcher, &PromiseWatcher::finished, watcher, &QObject::deleteLater);	// Self-destruct watcher object when it's no longer needed.
		watcher->setFuture(dataset()->makeSceneReady());
	}
}

/******************************************************************************
* Starts a timer to show the next animation frame.
******************************************************************************/
void AnimationSettings::scheduleNextAnimationFrame()
{
	if(!isPlaybackActive()) return;

	int timerSpeed = 1000;
	if(playbackSpeed() > 1) timerSpeed /= playbackSpeed();
	else if(playbackSpeed() < -1) timerSpeed *= -playbackSpeed();
	QTimer::singleShot(timerSpeed / framesPerSecond(), this, SLOT(onPlaybackTimer()));
}

/******************************************************************************
* Stops playback of the animation in the viewports.
******************************************************************************/
void AnimationSettings::stopAnimationPlayback()
{
	if(isPlaybackActive()) {
		_isPlaybackActive = false;
		Q_EMIT playbackChanged(_isPlaybackActive);
	}
}

/******************************************************************************
* Timer callback used during animation playback.
******************************************************************************/
void AnimationSettings::onPlaybackTimer()
{
	// Check if the animation playback has been deactivated in the meantime.
	if(!isPlaybackActive())
		return;

	// Add one frame to current time
	int newFrame = timeToFrame(time()) + 1;
	TimePoint newTime = frameToTime(newFrame);

	// Loop back to first frame if end has been reached.
	if(newTime > animationInterval().end()) {
		if(loopPlayback()) {
			newTime = animationInterval().start();
		}
		else {
			newTime = animationInterval().end();
			stopAnimationPlayback();
		}
	}

	// Set new time and continue playing.
	continuePlaybackAtTime(newTime);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
