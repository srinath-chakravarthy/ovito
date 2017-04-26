///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
 * Helper function that converts a string to a floating-point number.
 *****************************************************************************/
inline bool parseFloatType(const char* s, const char* s_end, float& f)
{
	const char* s_orig = s; // Make a copy, because parse() modifies its argument.
	if(!boost::spirit::qi::parse(s, s_end, boost::spirit::qi::float_, f)) {
		// Fall back to Boost's lexical cast if Boost.Spirit parser fails (e.g. for very small numbers like 1e-204).
#if BOOST_VERSION >= 105600
		if(!boost::conversion::try_lexical_convert(boost::make_iterator_range(s_orig, s_end), f))
			return false;
#else
		try { f = boost::lexical_cast<float>(s_orig, s_end - s_orig); }
		catch(const boost::bad_lexical_cast&) { return false; }
#endif
	}
	return true;
}

/******************************************************************************
 * Helper function that converts a string to a floating-point number.
 *****************************************************************************/
inline bool parseFloatType(const char* s, const char* s_end, double& f)
{
	const char* s_orig = s; // Make a copy, because parse() modifies its argument.
	if(!boost::spirit::qi::parse(s, s_end, boost::spirit::qi::double_, f)) {
		// Fall back to Boost's lexical cast if Boost.Spirit parser fails (e.g. for very small numbers like 1e-204).
#if BOOST_VERSION >= 105600
		if(!boost::conversion::try_lexical_convert(boost::make_iterator_range(s_orig, s_end), f))
			return false;
#else
		try { f = boost::lexical_cast<double>(s_orig, s_end - s_orig); }
		catch(const boost::bad_lexical_cast&) { return false; }
#endif
	}
	return true;
}

/******************************************************************************
 * Helper function that converts a string to an integer number.
 *****************************************************************************/
inline bool parseInt(const char* s, const char* s_end, int& i)
{
	return boost::spirit::qi::parse(s, s_end, boost::spirit::qi::int_, i);
}

/******************************************************************************
 * Helper function that converts a string repr. of a bool ('T' or 'F') to an int
 *****************************************************************************/
inline bool parseBool(const char* s, const char* s_end, int& d)
{
	if(s_end != s + 1) return false;
	if(s[0] == 'T') {
		d = 1;
		return true;
	}
	else if(s[0] == 'F') {
		d = 0;
		return true;
	}
	return false;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
