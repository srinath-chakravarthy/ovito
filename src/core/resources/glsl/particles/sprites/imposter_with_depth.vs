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

// Inputs from calling program:
uniform float basePointSize;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

#if __VERSION__ >= 130

	// The particle data:
	in vec3 particle_pos;
	in vec3 particle_color;
	in float particle_radius;

	// Output to fragment shader:
	flat out vec4 particle_color_out;
	flat out float depth_radius;		// The particle's radius.
	flat out float ze0;					// The particle's Z coordinate in eye coordinates.

#else

	attribute float particle_radius;
	//varying float ze0;					// The particle's Z coordinate in eye coordinates.
	//varying float depth_radius;			// The particle's radius.

#endif

void main()
{
#if __VERSION__ >= 130

	// Forward color to fragment shader.
	particle_color_out = vec4(particle_color, 1);

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * vec4(particle_pos, 1);

#else

	// Pass color to fragment shader.
	gl_FrontColor = gl_Color;

	// Transform and project particle position.
	vec4 eye_position = modelview_matrix * gl_Vertex;

#endif

	gl_Position = projection_matrix * eye_position;

	// Compute sprite size.		
	gl_PointSize = basePointSize * particle_radius / (eye_position.z * projection_matrix[2][3] + projection_matrix[3][3]);

#if __VERSION__ >= 130

	// Forward particle radius to fragment shader.
	depth_radius = particle_radius;
	
	// Pass particle position in eye coordinates to fragment shader.
	ze0 = eye_position.z;

#else

	// Forward particle radius to fragment shader.
	gl_FogFragCoord = particle_radius;
	
	// Pass particle position in eye coordinates to fragment shader.
	gl_FrontColor.a = eye_position.z;

#endif
}

