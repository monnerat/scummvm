/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * Additional copyright for this file:
 * Copyright (C) 1995-1997 Presto Studios, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "pegasus/pegasus.h"
#include "pegasus/notification.h"
#include "pegasus/timers.h"

namespace Pegasus {

Idler::Idler() {
	_isIdling = false;
}

Idler::~Idler() {
	stopIdling();
}

void Idler::startIdling() {
	if (!isIdling()) {
		((PegasusEngine *)g_engine)->addIdler(this);
		_isIdling = true;
	}
}

void Idler::stopIdling() {
	if (isIdling()) {
		((PegasusEngine *)g_engine)->removeIdler(this);
		_isIdling = false;
	}
}

TimeBase::TimeBase(const TimeScale preferredScale) {
	_preferredScale = preferredScale;
	_callBackList = 0;
	_paused = false;
	_flags = 0;
	((PegasusEngine *)g_engine)->addTimeBase(this);
}

TimeBase::~TimeBase() {
	if (_master)
		_master->_slaves.remove(this);

	((PegasusEngine *)g_engine)->removeTimeBase(this);
	disposeAllCallBacks();

	// TODO: Remove slaves? Make them remove themselves?
}

void TimeBase::setTime(const TimeValue time, const TimeScale scale) {
	// So, we're saying the *current* time from g_system->getMillis() is
	// what time/scale is. Which means we need to subtract time/scale from
	// the offset so that g_system->getMillis() - _currentOffset is
	// equal to time/scale.
	_timeOffset = g_system->getMillis() - time * 1000 / ((scale == 0) ? _preferredScale : scale);

	// TODO: Also adjust the slaves' offsets (once we're actually using the
	// masters for calculating time)
}

TimeValue TimeBase::getTime(const TimeScale scale) {
	if (_paused)
		return _pausedTime;

	Common::Rational normalTime = Common::Rational((g_system->getMillis() - _timeOffset) * ((scale == 0) ? _preferredScale : scale), 1000);

	// Adjust it according to our rate
	return (normalTime * getEffectiveRate()).toInt();
}

void TimeBase::setRate(const Common::Rational rate) {
	if (rate == 0) {
		_paused = false;
		_timeOffset = 0;
	} else if (_timeOffset != 0) {
		// Convert the time from the old rate to the new rate
		_timeOffset = g_system->getMillis() - (rate * (g_system->getMillis() - _timeOffset) / _rate).toInt();
	} else {
		// TODO: Check this
		setTime(_startTime, _startScale);
	}

	_rate = rate;
}

Common::Rational TimeBase::getEffectiveRate() const {
	return _rate * ((_master == 0) ? 1 : _master->getEffectiveRate());
}

void TimeBase::start() {
	if (_paused)
		_pausedRate = 1;
	else
		setRate(1);
}

void TimeBase::stop() {
	setRate(0);
	_paused = false;
}

void TimeBase::pause() {
	if (isRunning() && !_paused) {
		_pausedRate = getRate();
		stop();
		_pausedTime = getTime();
		_paused = true;
	}
}

void TimeBase::resume() {
	if (_paused) {
		setRate(_pausedRate);
		_paused = false;
	}
}

bool TimeBase::isRunning() {
	if (_paused && _pausedRate != 0)
		return true;

	Common::Rational rate = getRate();

	if (rate == 0)
		return false;

	if (getFlags() & kLoopTimeBase)
		return true;

	if (rate > 0)
		return getTime() < getStop();

	return getTime() > getStart();
}

void TimeBase::setStart(const TimeValue startTime, const TimeScale scale) {
	_startTime = startTime;
	_startScale = (scale == 0) ? _preferredScale : scale;
}

TimeValue TimeBase::getStart(const TimeScale scale) const {
	if (scale)
		return _startTime * scale / _startScale;

	return _startTime * _preferredScale / _startScale;
}

void TimeBase::setStop(const TimeValue stopTime, const TimeScale scale) {
	_stopTime = stopTime;
	_stopScale = (scale == 0) ? _preferredScale : scale;
}

TimeValue TimeBase::getStop(const TimeScale scale) const {
	if (scale)
		return _stopTime * scale / _stopScale;

	return _stopTime * _preferredScale / _stopScale;
}

void TimeBase::setSegment(const TimeValue startTime, const TimeValue stopTime, const TimeScale scale) {
	setStart(startTime, scale);
	setStop(stopTime, scale);
}

void TimeBase::getSegment(TimeValue &startTime, TimeValue &stopTime, const TimeScale scale) const {
	startTime = getStart(scale);
	stopTime = getStop(scale);
}

TimeValue TimeBase::getDuration(const TimeScale scale) const {
	TimeValue startTime, stopTime;
	getSegment(startTime, stopTime, scale);
	return stopTime - startTime;
}

void TimeBase::setMasterTimeBase(TimeBase *tb) {
	// TODO: We're just ignoring the master (except for effective rate)
	// for now to simplify things
	if (_master)
		_master->_slaves.remove(this);

	_master = tb;

	if (_master)
		_master->_slaves.push_back(this);
}

void TimeBase::checkCallBacks() {
	uint32 time = getTime();
	uint32 startTime = getStart();
	uint32 stopTime = getStop();

	for (TimeBaseCallBack *runner = _callBackList; runner != 0; runner = runner->_nextCallBack) {
		if (runner->_type == kCallBackAtTime && runner->_trigger == kTriggerTimeFwd) {
			if (time >= (runner->_param2 * _preferredScale / runner->_param3) && getRate() > 0)
				runner->callBack();
		} else if (runner->_type == kCallBackAtExtremes) {
			if (runner->_trigger == kTriggerAtStop) {
				if (time >= stopTime)
					runner->callBack();
			} else if (runner->_trigger == kTriggerAtStart) {
				if (time <= startTime)
					runner->callBack();
			}
		}
	}

	if (getFlags() & kLoopTimeBase) {
		// Loop
		if (time >= startTime)
			setTime(stopTime);
		else if (time <= stopTime)
			setTime(startTime);
	}
}

//	Protected functions only called by TimeBaseCallBack.

void TimeBase::addCallBack(TimeBaseCallBack *callBack) {
	callBack->_nextCallBack = _callBackList;
	_callBackList = callBack;
}

void TimeBase::removeCallBack(TimeBaseCallBack *callBack) {	
	if (_callBackList == callBack) {
		_callBackList = callBack->_nextCallBack;
	} else {
		TimeBaseCallBack *runner, *prevRunner;
	
		for (runner = _callBackList->_nextCallBack, prevRunner = _callBackList; runner != callBack; prevRunner = runner, runner = runner->_nextCallBack)
			;

		prevRunner->_nextCallBack = runner->_nextCallBack;
	}

	callBack->_nextCallBack = 0;
}

void TimeBase::disposeAllCallBacks() {
	TimeBaseCallBack *nextRunner;
	
	for (TimeBaseCallBack *runner = _callBackList; runner != 0; runner = nextRunner) {
		nextRunner = runner->_nextCallBack;
		runner->disposeCallBack();
		runner->_nextCallBack = 0;
	}

	_callBackList = 0;
}

TimeBaseCallBack::TimeBaseCallBack() {
	_timeBase = 0;
	_nextCallBack = 0;
	_trigger = kTriggerNone;
	_type = kCallBackNone;
}

TimeBaseCallBack::~TimeBaseCallBack() {
	releaseCallBack();
}

void TimeBaseCallBack::initCallBack(TimeBase *tb, CallBackType type) {
	releaseCallBack();
	_timeBase = tb;
	_timeBase->addCallBack(this);
	_type = type;
}

void TimeBaseCallBack::releaseCallBack() {
	if (_timeBase)
		_timeBase->removeCallBack(this);
	disposeCallBack();
}

void TimeBaseCallBack::disposeCallBack() {
	_timeBase = 0;
}

void TimeBaseCallBack::scheduleCallBack(CallBackTrigger trigger, uint32 param2, uint32 param3) {
	// TODO: Rename param2/param3?
	_trigger = trigger;
	_param2 = param2;
	_param3 = param3;
}

void TimeBaseCallBack::cancelCallBack() {
	_trigger = kTriggerNone;
}

IdlerTimeBase::IdlerTimeBase() {
	_lastTime = 0xffffffff;
	startIdling();
}

void IdlerTimeBase::useIdleTime() {	
	uint32 currentTime = getTime();
	if (currentTime != _lastTime) {
		_lastTime = currentTime;
		timeChanged(_lastTime);
	}
}

NotificationCallBack::NotificationCallBack() {
	_callBackFlag = 0;
	_notifier = 0;
}

void NotificationCallBack::callBack() {
	if (_notifier)
		_notifier->setNotificationFlags(_callBackFlag, _callBackFlag);
}

static const tNotificationFlags kFuseExpiredFlag = 1;

Fuse::Fuse() : _fuseNotification(0, (NotificationManager *)((PegasusEngine *)g_engine)) {
	_fuseNotification.notifyMe(this, kFuseExpiredFlag, kFuseExpiredFlag);
	_fuseCallBack.setNotification(&_fuseNotification);
	_fuseCallBack.initCallBack(&_fuseTimer, kCallBackAtExtremes);
	_fuseCallBack.setCallBackFlag(kFuseExpiredFlag);
}

void Fuse::primeFuse(const TimeValue frequency, const TimeScale scale) {
	stopFuse();
	_fuseTimer.setScale(scale);
	_fuseTimer.setSegment(0, frequency);
	_fuseTimer.setTime(0);
}

void Fuse::lightFuse() {
	if (!_fuseTimer.isRunning()) {
		_fuseCallBack.scheduleCallBack(kTriggerAtStop, 0, 0);
		_fuseTimer.start();
	}
}

void Fuse::stopFuse() {
	_fuseTimer.stop();
	_fuseCallBack.cancelCallBack();
	// Make sure the fuse has not triggered but not been caught yet...
	_fuseNotification.setNotificationFlags(0, 0xffffffff);
}

void Fuse::advanceFuse(const TimeValue time) {
	if (_fuseTimer.isRunning()) {
		_fuseTimer.stop();
		_fuseTimer.setTime(_fuseTimer.getTime() + time);
		_fuseTimer.start();
	}
}

TimeValue Fuse::getTimeRemaining() {
	return _fuseTimer.getStop() - _fuseTimer.getTime();
}

void Fuse::receiveNotification(Notification *, const tNotificationFlags) {
	stopFuse();
	invokeAction();
}

} // End of namespace Pegasus
