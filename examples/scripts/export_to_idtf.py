""" 
    This OVITO script exports an OVITO scene to the IDTF file format, 
    which can be converted to the u3d format, which in turn can be 
    embedded in PDF documents.
    
    Usage:
    
       ovitos -o scenefile.ovito export_to_idtf.py > output.idtf
"""
import sys

import ovito
from ovito.data import *
from ovito.vis import *

print('FILE_FORMAT "IDTF"')
print('FORMAT_VERSION 100')

# Global dictionary that keeps track of all unique colors and the objects that use it.
color_list = {}

def export_particle(parent_node_name, index, pos, color, radius):
    node_name = parent_node_name + '.' + str(index)
    # Check if a particle with the same color has already been exported.
    # If not, add color to the list of unique colors.
    if color in color_list:
        color_list[color][1].append(node_name)
    else:
        color_list[color] = (len(color_list), [node_name])
    print('NODE "MODEL" {')
    print('  NODE_NAME "%s"' % node_name)
    print('  PARENT_LIST {')
    print('    PARENT_COUNT 1')
    print('    PARENT 0 {')
    print('      PARENT_NAME "%s"' % parent_node_name)
    print('      PARENT_TM {')
    print('        %f 0 0 0' % radius)
    print('        0 %f 0 0' % radius)
    print('        0 0 %f 0' % radius)
    print('        %f %f %f 1' % (pos[0], pos[1], pos[2]))
    print('      }')
    print('    }')
    print('  }')
    print('  RESOURCE_NAME "SphereModel"')
    print('}')

def export_particles(node_name, data):
    positions = data.particle_properties.position.array
    particle_display = data.particle_properties.position.display
    assert(particle_display.shape == ParticleDisplay.Shape.Sphere)  # Only spherical particles can be exported at the moment.
    
    color = (1,0,0)
    color_property = None
    if 'Color' in data.particle_properties: color_property = data.particle_properties.color.array
    
    radius = particle_display.radius
    radius_property = None
    if 'Radius' in data.particle_properties: radius_property = data.particle_properties.radius.array
    
    type_property = None
    if 'Particle Type' in data.particle_properties: 
        type_property = data.particle_properties.particle_type.array
        particle_type_colors = {}
        for t in data.particle_properties.particle_type.type_list:
            particle_type_colors[t.id] = t.color    
    
    for i in range(len(positions)):
        
        if type_property != None: color = particle_type_colors[type_property[i]]
        if color_property != None: color = tuple(color_property[i])
        if radius_property != None: radius = radius_property[i]
        
        export_particle(node_name, i, positions[i], color, radius)

def export_simulation_cell(node_name, cell):
    print('NODE "MODEL" {')
    print('  NODE_NAME "%s.cell"' % node_name)
    print('  PARENT_LIST {')
    print('    PARENT_COUNT 1')
    print('    PARENT 0 {')
    print('      PARENT_NAME "%s"' % node_name)
    print('      PARENT_TM {')
    print('        %f %f %f 0' % (cell.matrix.get(0,0), cell.matrix.get(1,0), cell.matrix.get(2,0)))
    print('        %f %f %f 0' % (cell.matrix.get(0,1), cell.matrix.get(1,1), cell.matrix.get(2,1)))
    print('        %f %f %f 0' % (cell.matrix.get(0,2), cell.matrix.get(1,2), cell.matrix.get(2,2)))
    print('        %f %f %f 1' % (cell.matrix.get(0,3), cell.matrix.get(1,3), cell.matrix.get(2,3)))
    print('      }')
    print('    }')
    print('  }')
    print('  RESOURCE_NAME "Box"')
    print('}')

def export_node(node):
    group_name = node.objectTitle
    print('NODE "GROUP" {')
    print('  NODE_NAME "%s"' % group_name)
    print('  PARENT_LIST {')
    print('    PARENT_COUNT 1')
    print('    PARENT 0 {')
    print('      PARENT_NAME ""')
    print('      PARENT_TM {')
    print('        1 0 0 0')
    print('        0 1 0 0')
    print('        0 0 1 0')
    print('        0 0 0 1')
    print('      }')
    print('    }')
    print('  }')
    print('}')

    data = node.compute()
    
    if data.number_of_particles != 0 and data.particle_properties.position.display.enabled:
        export_particles(group_name, data)
        
    if 'Simulation cell' in data and data.cell.display.enabled:
        export_simulation_cell(group_name, data.cell)

# Loop over scene nodes.
for i in range(len(ovito.dataset.scene_nodes)):
    export_node(ovito.dataset.scene_nodes[i])
    
# Write node resources.

print('RESOURCE_LIST "MODEL" {')
print('  RESOURCE_COUNT 1')
print('  RESOURCE 0 {')
print('    RESOURCE_NAME "SphereModel"')
print('    MODEL_TYPE "MESH"')
print('    MESH {')
print('      FACE_COUNT 320')
print('      MODEL_POSITION_COUNT 162')
print('      MODEL_NORMAL_COUNT 162')
print('      MODEL_DIFFUSE_COLOR_COUNT 0')
print('      MODEL_SPECULAR_COLOR_COUNT 0')
print('      MODEL_TEXTURE_COORD_COUNT 0')
print('      MODEL_BONE_COUNT 0')
print('      MODEL_SHADING_COUNT 1')
print('      MODEL_SHADING_DESCRIPTION_LIST {')
print('        SHADING_DESCRIPTION 0 {')
print('          TEXTURE_LAYER_COUNT 0')
print('          SHADER_ID 0')
print('        }')
print('      }')
print('      MESH_FACE_POSITION_LIST { 0 42 44 12 43 42 14 44 43 44 42 43 1 45 47 13 46 45 12 47 46 47 45 46 2 48 50 14 49 48 13 50 49 50 48 49 14 43 49 12 46 43 13 49 46 49 43 46 0 44 52 14 51 44 16 52 51 52 44 51 2 53 48 15 54 53 14 48 54 48 53 54 3 55 57 16 56 55 15 57 56 57 55 56 16 51 56 14 54 51 15 56 54 56 51 54 0 52 59 16 58 52 18 59 58 59 52 58 3 60 55 17 61 60 16 55 61 55 60 61 4 62 64 18 63 62 17 64 63 64 62 63 18 58 63 16 61 58 17 63 61 63 58 61 0 59 66 18 65 59 20 66 65 66 59 65 4 67 62 19 68 67 18 62 68 62 67 68 5 69 71 20 70 69 19 71 70 71 69 70 20 65 70 18 68 65 19 70 68 70 65 68 0 66 42 20 72 66 12 42 72 42 66 72 5 73 69 21 74 73 20 69 74 69 73 74 1 47 76 12 75 47 21 76 75 76 47 75 12 72 75 20 74 72 21 75 74 75 72 74 1 77 45 22 78 77 13 45 78 45 77 78 6 79 81 23 80 79 22 81 80 81 79 80 2 50 83 13 82 50 23 83 82 83 50 82 13 78 82 22 80 78 23 82 80 82 78 80 2 84 53 24 85 84 15 53 85 53 84 85 7 86 88 25 87 86 24 88 87 88 86 87 3 57 90 15 89 57 25 90 89 90 57 89 15 85 89 24 87 85 25 89 87 89 85 87 3 91 60 26 92 91 17 60 92 60 91 92 8 93 95 27 94 93 26 95 94 95 93 94 4 64 97 17 96 64 27 97 96 97 64 96 17 92 96 26 94 92 27 96 94 96 92 94 4 98 67 28 99 98 19 67 99 67 98 99 9 100 102 29 101 100 28 102 101 102 100 101 5 71 104 19 103 71 29 104 103 104 71 103 19 99 103 28 101 99 29 103 101 103 99 101 5 105 73 30 106 105 21 73 106 73 105 106 10 107 109 31 108 107 30 109 108 109 107 108 1 76 111 21 110 76 31 111 110 111 76 110 21 106 110 30 108 106 31 110 108 110 106 108 6 81 113 22 112 81 32 113 112 113 81 112 1 111 77 31 114 111 22 77 114 77 111 114 10 115 107 32 116 115 31 107 116 107 115 116 32 112 116 22 114 112 31 116 114 116 112 114 7 88 118 24 117 88 33 118 117 118 88 117 2 83 84 23 119 83 24 84 119 84 83 119 6 120 79 33 121 120 23 79 121 79 120 121 33 117 121 24 119 117 23 121 119 121 117 119 8 95 123 26 122 95 34 123 122 123 95 122 3 90 91 25 124 90 26 91 124 91 90 124 7 125 86 34 126 125 25 86 126 86 125 126 34 122 126 26 124 122 25 126 124 126 122 124 9 102 128 28 127 102 35 128 127 128 102 127 4 97 98 27 129 97 28 98 129 98 97 129 8 130 93 35 131 130 27 93 131 93 130 131 35 127 131 28 129 127 27 131 129 131 127 129 10 109 133 30 132 109 36 133 132 133 109 132 5 104 105 29 134 104 30 105 134 105 104 134 9 135 100 36 136 135 29 100 136 100 135 136 36 132 136 30 134 132 29 136 134 136 132 134 11 137 139 37 138 137 38 139 138 139 137 138 6 113 141 32 140 113 37 141 140 141 113 140 10 142 115 38 143 142 32 115 143 115 142 143 38 138 143 37 140 138 32 143 140 143 138 140 11 144 137 39 145 144 37 137 145 137 144 145 7 118 147 33 146 118 39 147 146 147 118 146 6 141 120 37 148 141 33 120 148 120 141 148 37 145 148 39 146 145 33 148 146 148 145 146 11 149 144 40 150 149 39 144 150 144 149 150 8 123 152 34 151 123 40 152 151 152 123 151 7 147 125 39 153 147 34 125 153 125 147 153 39 150 153 40 151 150 34 153 151 153 150 151 11 154 149 41 155 154 40 149 155 149 154 155 9 128 157 35 156 128 41 157 156 157 128 156 8 152 130 40 158 152 35 130 158 130 152 158 40 155 158 41 156 155 35 158 156 158 155 156 11 139 154 38 159 139 41 154 159 154 139 159 10 133 142 36 160 133 38 142 160 142 133 160 9 157 135 41 161 157 36 135 161 135 157 161 41 159 161 38 160 159 36 161 160 161 159 160 }')
print('      MESH_FACE_NORMAL_LIST { 0 42 44 12 43 42 14 44 43 44 42 43 1 45 47 13 46 45 12 47 46 47 45 46 2 48 50 14 49 48 13 50 49 50 48 49 14 43 49 12 46 43 13 49 46 49 43 46 0 44 52 14 51 44 16 52 51 52 44 51 2 53 48 15 54 53 14 48 54 48 53 54 3 55 57 16 56 55 15 57 56 57 55 56 16 51 56 14 54 51 15 56 54 56 51 54 0 52 59 16 58 52 18 59 58 59 52 58 3 60 55 17 61 60 16 55 61 55 60 61 4 62 64 18 63 62 17 64 63 64 62 63 18 58 63 16 61 58 17 63 61 63 58 61 0 59 66 18 65 59 20 66 65 66 59 65 4 67 62 19 68 67 18 62 68 62 67 68 5 69 71 20 70 69 19 71 70 71 69 70 20 65 70 18 68 65 19 70 68 70 65 68 0 66 42 20 72 66 12 42 72 42 66 72 5 73 69 21 74 73 20 69 74 69 73 74 1 47 76 12 75 47 21 76 75 76 47 75 12 72 75 20 74 72 21 75 74 75 72 74 1 77 45 22 78 77 13 45 78 45 77 78 6 79 81 23 80 79 22 81 80 81 79 80 2 50 83 13 82 50 23 83 82 83 50 82 13 78 82 22 80 78 23 82 80 82 78 80 2 84 53 24 85 84 15 53 85 53 84 85 7 86 88 25 87 86 24 88 87 88 86 87 3 57 90 15 89 57 25 90 89 90 57 89 15 85 89 24 87 85 25 89 87 89 85 87 3 91 60 26 92 91 17 60 92 60 91 92 8 93 95 27 94 93 26 95 94 95 93 94 4 64 97 17 96 64 27 97 96 97 64 96 17 92 96 26 94 92 27 96 94 96 92 94 4 98 67 28 99 98 19 67 99 67 98 99 9 100 102 29 101 100 28 102 101 102 100 101 5 71 104 19 103 71 29 104 103 104 71 103 19 99 103 28 101 99 29 103 101 103 99 101 5 105 73 30 106 105 21 73 106 73 105 106 10 107 109 31 108 107 30 109 108 109 107 108 1 76 111 21 110 76 31 111 110 111 76 110 21 106 110 30 108 106 31 110 108 110 106 108 6 81 113 22 112 81 32 113 112 113 81 112 1 111 77 31 114 111 22 77 114 77 111 114 10 115 107 32 116 115 31 107 116 107 115 116 32 112 116 22 114 112 31 116 114 116 112 114 7 88 118 24 117 88 33 118 117 118 88 117 2 83 84 23 119 83 24 84 119 84 83 119 6 120 79 33 121 120 23 79 121 79 120 121 33 117 121 24 119 117 23 121 119 121 117 119 8 95 123 26 122 95 34 123 122 123 95 122 3 90 91 25 124 90 26 91 124 91 90 124 7 125 86 34 126 125 25 86 126 86 125 126 34 122 126 26 124 122 25 126 124 126 122 124 9 102 128 28 127 102 35 128 127 128 102 127 4 97 98 27 129 97 28 98 129 98 97 129 8 130 93 35 131 130 27 93 131 93 130 131 35 127 131 28 129 127 27 131 129 131 127 129 10 109 133 30 132 109 36 133 132 133 109 132 5 104 105 29 134 104 30 105 134 105 104 134 9 135 100 36 136 135 29 100 136 100 135 136 36 132 136 30 134 132 29 136 134 136 132 134 11 137 139 37 138 137 38 139 138 139 137 138 6 113 141 32 140 113 37 141 140 141 113 140 10 142 115 38 143 142 32 115 143 115 142 143 38 138 143 37 140 138 32 143 140 143 138 140 11 144 137 39 145 144 37 137 145 137 144 145 7 118 147 33 146 118 39 147 146 147 118 146 6 141 120 37 148 141 33 120 148 120 141 148 37 145 148 39 146 145 33 148 146 148 145 146 11 149 144 40 150 149 39 144 150 144 149 150 8 123 152 34 151 123 40 152 151 152 123 151 7 147 125 39 153 147 34 125 153 125 147 153 39 150 153 40 151 150 34 153 151 153 150 151 11 154 149 41 155 154 40 149 155 149 154 155 9 128 157 35 156 128 41 157 156 157 128 156 8 152 130 40 158 152 35 130 158 130 152 158 40 155 158 41 156 155 35 158 156 158 155 156 11 139 154 38 159 139 41 154 159 154 139 159 10 133 142 36 160 133 38 142 160 142 133 160 9 157 135 41 161 157 36 135 161 135 157 161 41 159 161 38 160 159 36 161 160 161 159 160 }')
print('      MESH_FACE_SHADING_LIST { 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 }')
print('      MODEL_POSITION_LIST { 0 0 1 .894 0 .447 .276 .851 .447 -.724 .526 .447 -.724 -.526 .447 .276 -.851 .447 .724 .526 -.447 -.276 .851 -.447 -.894 .0 -.447 -.276 -.851 -.447 .724 -.526 -.447 0 0 -1 .526 0 .851 .688 .5 .526 .162 .5 .851 -.263 .809 .526 -.425 .309 .851 -.851 .0 .526 -.425 -.309 .851 -.263 -.809 .526 .162 -.5 .851 .688 -.5 .526 .951 .309 0 .588 .809 0 .0 1 0 -.588 .809 0 -.951 .309 0 -.951 -.309 0 -.588 -.809 0 .0 -1 0 .588 -.809 0 .951 -.309 0 .851 .0 -.526 .263 .809 -.526 -.688 .5 -.526 -.688 -.5 -.526 .263 -.809 -.526 .425 .309 -.851 .425 -.309 -.851 -.162 .5 -.851 -.526 .0 -.851 -.162 -.5 -.851 .273 0 .962 .362 .263 .894 .084 .26 .962 .823 .26 .506 .638 .263 .724 .738 0 .675 .228 .702 .675 .447 .526 .724 .501 .702 .506 -.138 .425 .894 -.221 .161 .962 .007 .863 .506 -.053 .688 .724 -.597 .434 .675 -.362 .588 .724 -.513 .694 .506 -.447 .0 .894 -.221 -.161 .962 -.818 .273 .506 -.671 .162 .724 -.597 -.434 .675 -.671 -.162 .724 -.818 -.273 .506 -.138 -.425 .894 .084 -.26 .962 -.513 -.694 .506 -.362 -.588 .724 .228 -.702 .675 -.053 -.688 .724 .007 -.863 .506 .362 -.263 .894 .501 -.702 .506 .447 -.526 .724 .638 -.263 .724 .823 -.26 .506 .959 .161 .232 .862 .425 .276 .682 .694 -.232 .809 .588 0 .87 .434 -.232 .671 .688 .276 .449 .863 .232 .144 .962 .232 -.138 .951 .276 -.449 .863 -.232 -.309 .951 0 -.144 .962 -.232 -.447 .851 .276 -.682 .694 .232 -.87 .434 .232 -.947 .162 .276 -.959 -.161 -.232 -1 .0 0 -.959 .161 -.232 -.947 -.162 .276 -.87 -.434 .232 -.682 -.694 .232 -.447 -.851 .276 -.144 -.962 -.232 -.309 -.951 0 -.449 -.863 -.232 -.138 -.951 .276 .144 -.962 .232 .449 -.863 .232 .671 -.688 .276 .87 -.434 -.232 .809 -.588 0 .682 -.694 -.232 .862 -.425 .276 .959 -.161 .232 .947 .162 -.276 .818 .273 -.506 1 .0 0 .818 -.273 -.506 .947 -.162 -.276 .138 .951 -.276 -.007 .863 -.506 .309 .951 0 .513 .694 -.506 .447 .851 -.276 -.862 .425 -.276 -.823 .26 -.506 -.809 .588 0 -.501 .702 -.506 -.671 .688 -.276 -.671 -.688 -.276 -.501 -.702 -.506 -.809 -.588 0 -.823 -.26 -.506 -.862 -.425 -.276 .447 -.851 -.276 .513 -.694 -.506 .309 -.951 0 -.007 -.863 -.506 .138 -.951 -.276 .221 .161 -.962 .447 .0 -.894 .221 -.161 -.962 .671 .162 -.724 .597 .434 -.675 .597 -.434 -.675 .671 -.162 -.724 -.084 .26 -.962 .138 .425 -.894 .053 .688 -.724 -.228 .702 -.675 .362 .588 -.724 -.273 .0 -.962 -.362 .263 -.894 -.638 .263 -.724 -.738 .0 -.675 -.447 .526 -.724 -.084 -.26 -.962 -.362 -.263 -.894 -.447 -.526 -.724 -.228 -.702 -.675 -.638 -.263 -.724 .138 -.425 -.894 .362 -.588 -.724 .053 -.688 -.724 }')
print('      MODEL_NORMAL_LIST { 0 0 1 .894 0 .447 .276 .851 .447 -.724 .526 .447 -.724 -.526 .447 .276 -.851 .447 .724 .526 -.447 -.276 .851 -.447 -.894 .0 -.447 -.276 -.851 -.447 .724 -.526 -.447 0 0 -1 .526 0 .851 .688 .5 .526 .162 .5 .851 -.263 .809 .526 -.425 .309 .851 -.851 .0 .526 -.425 -.309 .851 -.263 -.809 .526 .162 -.5 .851 .688 -.5 .526 .951 .309 0 .588 .809 0 .0 1 0 -.588 .809 0 -.951 .309 0 -.951 -.309 0 -.588 -.809 0 .0 -1 0 .588 -.809 0 .951 -.309 0 .851 .0 -.526 .263 .809 -.526 -.688 .5 -.526 -.688 -.5 -.526 .263 -.809 -.526 .425 .309 -.851 .425 -.309 -.851 -.162 .5 -.851 -.526 .0 -.851 -.162 -.5 -.851 .273 0 .962 .362 .263 .894 .084 .26 .962 .823 .26 .506 .638 .263 .724 .738 0 .675 .228 .702 .675 .447 .526 .724 .501 .702 .506 -.138 .425 .894 -.221 .161 .962 .007 .863 .506 -.053 .688 .724 -.597 .434 .675 -.362 .588 .724 -.513 .694 .506 -.447 .0 .894 -.221 -.161 .962 -.818 .273 .506 -.671 .162 .724 -.597 -.434 .675 -.671 -.162 .724 -.818 -.273 .506 -.138 -.425 .894 .084 -.26 .962 -.513 -.694 .506 -.362 -.588 .724 .228 -.702 .675 -.053 -.688 .724 .007 -.863 .506 .362 -.263 .894 .501 -.702 .506 .447 -.526 .724 .638 -.263 .724 .823 -.26 .506 .959 .161 .232 .862 .425 .276 .682 .694 -.232 .809 .588 0 .87 .434 -.232 .671 .688 .276 .449 .863 .232 .144 .962 .232 -.138 .951 .276 -.449 .863 -.232 -.309 .951 0 -.144 .962 -.232 -.447 .851 .276 -.682 .694 .232 -.87 .434 .232 -.947 .162 .276 -.959 -.161 -.232 -1 .0 0 -.959 .161 -.232 -.947 -.162 .276 -.87 -.434 .232 -.682 -.694 .232 -.447 -.851 .276 -.144 -.962 -.232 -.309 -.951 0 -.449 -.863 -.232 -.138 -.951 .276 .144 -.962 .232 .449 -.863 .232 .671 -.688 .276 .87 -.434 -.232 .809 -.588 0 .682 -.694 -.232 .862 -.425 .276 .959 -.161 .232 .947 .162 -.276 .818 .273 -.506 1 .0 0 .818 -.273 -.506 .947 -.162 -.276 .138 .951 -.276 -.007 .863 -.506 .309 .951 0 .513 .694 -.506 .447 .851 -.276 -.862 .425 -.276 -.823 .26 -.506 -.809 .588 0 -.501 .702 -.506 -.671 .688 -.276 -.671 -.688 -.276 -.501 -.702 -.506 -.809 -.588 0 -.823 -.26 -.506 -.862 -.425 -.276 .447 -.851 -.276 .513 -.694 -.506 .309 -.951 0 -.007 -.863 -.506 .138 -.951 -.276 .221 .161 -.962 .447 .0 -.894 .221 -.161 -.962 .671 .162 -.724 .597 .434 -.675 .597 -.434 -.675 .671 -.162 -.724 -.084 .26 -.962 .138 .425 -.894 .053 .688 -.724 -.228 .702 -.675 .362 .588 -.724 -.273 .0 -.962 -.362 .263 -.894 -.638 .263 -.724 -.738 .0 -.675 -.447 .526 -.724 -.084 -.26 -.962 -.362 -.263 -.894 -.447 -.526 -.724 -.228 -.702 -.675 -.638 -.263 -.724 .138 -.425 -.894 .362 -.588 -.724 .053 -.688 -.724 }')
print('    }')
print('  }')
print('}')
    
# Write model resource for simulation cell.
print('RESOURCE_LIST "MODEL" {')
print('  RESOURCE_COUNT 1')
print('  RESOURCE 0 {')
print('    RESOURCE_NAME "Box"')
print('    MODEL_TYPE "LINE_SET"')
print('    LINE_SET {')
print('      LINE_COUNT 12')
print('      MODEL_POSITION_COUNT 8')
print('      MODEL_NORMAL_COUNT 1')
print('      MODEL_DIFFUSE_COLOR_COUNT 0')
print('      MODEL_SPECULAR_COLOR_COUNT 0')
print('      MODEL_TEXTURE_COORD_COUNT 0')
print('      MODEL_SHADING_COUNT 1')
print('      MODEL_SHADING_DESCRIPTION_LIST {')
print('        SHADING_DESCRIPTION 0 {')
print('          TEXTURE_LAYER_COUNT 0')
print('          SHADER_ID 1')
print('        }')
print('      }')
print('      LINE_POSITION_LIST { 0 4 1 5 2 6 3 7 0 1 1 2 2 3 3 0 4 5 5 6 6 7 7 4 }')
print('      LINE_NORMAL_LIST { 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 }')
print('      LINE_SHADING_LIST { 0 0 0 0 0 0 0 0 0 0 0 0 }')
print('      MODEL_POSITION_LIST { 0 0 0 1 0 0 1 1 0 0 1 0 0 0 1 1 0 1 1 1 1 0 1 1 }')
print('      MODEL_NORMAL_LIST { 0 0 1 }')
print('    }')
print('  }')
print('}')

# Generate one shader/material pair per unique particle color.
print('RESOURCE_LIST "SHADER" {')
print('  RESOURCE_COUNT %i' % len(color_list))
for index,color_entry in enumerate(color_list.values()):
    print('  RESOURCE %i {' % index)
    print('    RESOURCE_NAME "ColorShader_%i"' % color_entry[0])
    print('    ATTRIBUTE_USE_VERTEX_COLOR "FALSE"')
    print('    SHADER_MATERIAL_NAME "ColorMaterial_%i"' % color_entry[0])
    print('    SHADER_ACTIVE_TEXTURE_COUNT 0')
    print('  }')
print('}')

print('RESOURCE_LIST "MATERIAL" {')
print('  RESOURCE_COUNT %i' % len(color_list))
for index,item in enumerate(color_list.items()):
    print('  RESOURCE %i {' % index)
    print('    RESOURCE_NAME "ColorMaterial_%i"' % item[1][0])
    print('    MATERIAL_AMBIENT %f %f %f' % (item[0][0], item[0][1], item[0][2]))
    print('    MATERIAL_DIFFUSE %f %f %f' % (item[0][0], item[0][1], item[0][2]))
    print('    MATERIAL_SPECULAR 0.0 0.0 0.0')
    print('    MATERIAL_EMISSIVE 0.0 0.0 0.0')
    print('    MATERIAL_REFLECTIVITY 0.0')
    print('    MATERIAL_OPACITY 1')
    print('  }')
print('}')

for index,color_entry in enumerate(color_list.values()):
    for obj in color_entry[1]:
        print('MODIFIER "SHADING" {')
        print('  MODIFIER_NAME "%s"' % obj)
        print('  PARAMETERS {')
        print('    SHADER_LIST_COUNT 1')
        print('    SHADING_GROUP {')
        print('      SHADER_LIST 0 {')
        print('        SHADER_COUNT 1')
        print('        SHADER_NAME_LIST {')
        print('          SHADER 0 NAME: "ColorShader_%i"' % color_entry[0])
        print('        }')
        print('      }')
        print('    }')
        print('  }')
        print('}')
    
