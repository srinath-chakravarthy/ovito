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
#include <core/app/Application.h>
#include "Exception.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util)

Exception::Exception(QObject* context) : _context(context)
{
	_messages.push_back("An exception has occurred.");
}

Exception::Exception(const QString& message, QObject* context) : _context(context)
{
	_messages.push_back(message);
}

Exception::Exception(const QStringList& errorMessages, QObject* context) : _messages(errorMessages), _context(context)
{
}

Exception& Exception::appendDetailMessage(const QString& message)
{
	_messages.push_back(message);
	return *this;
}

Exception& Exception::prependGeneralMessage(const QString& message)
{
	_messages.push_front(message);
	return *this;
}

void Exception::logError() const
{
	for(const QString& msg : _messages)
		qCritical("%s", qPrintable(msg));
}

void Exception::reportError(bool blocking) const
{
	Application* app = Application::instance();
	if(!app)
		logError();
	else
		app->reportError(*this, blocking);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
