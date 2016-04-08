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

#ifndef __OVITO_PROGRESS_DISPLAY_H
#define __OVITO_PROGRESS_DISPLAY_H

#include <core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class AbstractProgressDisplay
{
public:

	/// Returns whether the operation has been canceled by the user.
	virtual bool wasCanceled() = 0;

	/// Cancels the operation.
	virtual void cancel() = 0;

	/// Sets the status text to be displayed.
	virtual void setStatusText(const QString& text) = 0;

	/// Return the current status text.
	virtual QString statusText() = 0;

	/// Returns the highest value represented by the progress bar.
	virtual int maximum() = 0;

	/// Sets the highest value represented by the progress bar.
	virtual void setMaximum(int max) = 0;

	/// Returns the value displayed by the progress bar.
	virtual int value() = 0;

	/// Sets the value displayed by the progress bar.
	virtual void setValue(int v) = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_PROGRESS_DISPLAY_H
