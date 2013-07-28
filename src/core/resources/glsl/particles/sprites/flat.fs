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

/***********************************************************************
 * This OpenGL fragment shader renders a flat atom without shading.
 ***********************************************************************/

flat in vec4 particle_color_out;

out vec4 FragColor;

void main()
{
	vec2 shifted_coords = gl_PointCoord - vec2(0.5, 0.5);
	if(dot(shifted_coords, shifted_coords) >= 0.25) discard;
	FragColor = particle_color_out;
}