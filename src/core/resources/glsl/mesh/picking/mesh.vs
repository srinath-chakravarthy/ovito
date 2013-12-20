///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
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

uniform mat4 modelview_projection_matrix;
uniform int pickingBaseID;

#if __VERSION__ >= 130

	// The input data:
	in vec3 position;
	
	// Output passed to fragment shader.
	flat out vec4 vertex_color_fs;

#else

	// The input data:
	attribute float vertexID;
	#define gl_VertexID int(vertexID)
	
	// Output passed to fragment shader.
	#define vertex_color_fs gl_FrontColor

#endif

void main()
{
	// Compute color from object ID.
	int objectID = pickingBaseID + gl_VertexID / 3;
#if __VERSION__ >= 130
	vertex_color_fs = vec4(
		float(objectID & 0xFF) / 255.0, 
		float((objectID >> 8) & 0xFF) / 255.0, 
		float((objectID >> 16) & 0xFF) / 255.0, 
		float((objectID >> 24) & 0xFF) / 255.0);		

	gl_Position = modelview_projection_matrix * vec4(position, 1.0);
#else
	vertex_color_fs = vec4(
		float(mod(objectID, 0x100)) / 255.0, 
		float(mod(objectID / 0x100, 0x100)) / 255.0, 
		float(mod(objectID / 0x10000, 0x100)) / 255.0, 
		float(mod(objectID / 0x1000000, 0x100)) / 255.0);		

	gl_Position = modelview_projection_matrix * gl_Vertex;
#endif
}
