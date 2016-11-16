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

#ifndef __OVITO_DEFERRED_METHOD_INVOCATION_H
#define __OVITO_DEFERRED_METHOD_INVOCATION_H

#include <core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * Utility class that invokes a member function of an object at some later time.
 * While another invocation is aleady queued, new calls will be ignored.
 *
 * The DeferredMethodInvocation class can be used to compress rapid update signals
 * into a single call to a widget's repaint method.
 */
template<typename ObjectClass, void (ObjectClass::*method)()>
class DeferredMethodInvocation 
{
public:

	void operator()(ObjectClass* obj) {
		struct Event : public QEvent {
			DeferredMethodInvocation* owner;
			ObjectClass* object;
			Event(DeferredMethodInvocation* owner, ObjectClass* object) : QEvent(QEvent::None), owner(owner), object(object) {}
			~Event() {
				owner->_callPending = false;
				(object->*method)();
			}
		};
		if(!_callPending) {
			_callPending = true;
			QCoreApplication::postEvent(obj, new Event(this, obj));
		}
	}

private:

	bool _callPending = false;
};


OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_DEFERRED_METHOD_INVOCATION_H

