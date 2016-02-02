from ovito.io import import_file
import numpy.linalg

node = import_file("simulation.dump")
cell = node.source.cell

# Compute simulation cell volume by taking the determinant of the
# left 3x3 submatrix:
volume = abs(numpy.linalg.det(cell.matrix[0:3,0:3]))

# Make cell twice as large along the Y direction by scaling the
# second cell vector.
mat = cell.matrix.copy()
mat[:,1] = mat[:,1] * 2.0
cell.matrix = mat

# Change color of simulation cell to red:
cell.display.rendering_color = (1.0, 0.0, 0.0)