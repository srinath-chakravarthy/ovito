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

#ifndef __OVITO_OPENGL_HELPERS_H
#define __OVITO_OPENGL_HELPERS_H

#include <core/Core.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

// The minimum OpenGL version required by Ovito:
#define OVITO_OPENGL_MINIMUM_VERSION_MAJOR 			2
#define OVITO_OPENGL_MINIMUM_VERSION_MINOR			1

// The standard OpenGL version used by Ovito:
#define OVITO_OPENGL_REQUESTED_VERSION_MAJOR 		3
#define OVITO_OPENGL_REQUESTED_VERSION_MINOR		2

/// Reports OpenGL error status codes.
extern OVITO_CORE_EXPORT void checkOpenGLErrorStatus(const char* command, const char* sourceFile, int sourceLine);

// OpenGL debugging macro:
#ifdef OVITO_DEBUG
	#define OVITO_CHECK_OPENGL(cmd)									\
	{																\
		cmd;														\
		Ovito::checkOpenGLErrorStatus(#cmd, __FILE__, __LINE__);	\
	}
    #define OVITO_REPORT_OPENGL_ERRORS() Ovito::checkOpenGLErrorStatus("", __FILE__, __LINE__);
#else
	#define OVITO_CHECK_OPENGL(cmd)			cmd
    #define OVITO_REPORT_OPENGL_ERRORS()
#endif

// Type-specific OpenGL functions:
#ifdef FLOATTYPE_FLOAT
	inline void glVertex3(FloatType x, FloatType y, FloatType z) { glVertex3f(x,y,z); }
	inline void glColor3(FloatType r, FloatType g, FloatType b) { glColor3f(r,g,b); }
#else
	inline void glVertex3(FloatType x, FloatType y, FloatType z) { glVertex3d(x,y,z); }
	inline void glColor3(FloatType r, FloatType g, FloatType b) { glColor3d(r,g,b); }
#endif

// Type-specific OpenGL functions:
inline void glVertex(const Point_3<GLdouble>& v) { glVertex3dv(v.data()); }
inline void glVertex(const Point_3<GLfloat>& v) { glVertex3fv(v.data()); }
inline void glVertex(const Point_2<GLdouble>& v) { glVertex2dv(v.data()); }
inline void glVertex(const Point_2<GLfloat>& v) { glVertex2fv(v.data()); }
inline void glVertex(const Vector_3<GLdouble>& v) { glVertex3dv(v.data()); }
inline void glVertex(const Vector_3<GLfloat>& v) { glVertex3fv(v.data()); }
inline void glVertex(const Vector_4<GLdouble>& v) { glVertex4dv(v.data()); }
inline void glVertex(const Vector_4<GLfloat>& v) { glVertex4fv(v.data()); }
inline void glLoadMatrix(const Matrix_4<GLdouble>& tm) { glLoadMatrixd(tm.elements()); }
inline void glLoadMatrix(const Matrix_4<GLfloat>& tm) { glLoadMatrixf(tm.elements()); }
inline void glColor3(const ColorT<GLdouble>& c) { glColor3dv(c.data()); }
inline void glColor3(const ColorT<GLfloat>& c) { glColor3fv(c.data()); }
inline void glColor4(const ColorAT<GLdouble>& c) { glColor4dv(c.data()); }
inline void glColor4(const ColorAT<GLfloat>& c) { glColor4fv(c.data()); }

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_OPENGL_HELPERS_H
