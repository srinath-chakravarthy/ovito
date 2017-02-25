from ovito.io import *
from ovito.modifiers import *
import numpy

node = import_file("../../files/NetCDF/sheared_aSi.nc")

import ovito.modifiers
ovito.modifiers.BinAndReduceModifier(property = "Position.X")

modifier = BinAndReduceModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  bin_count_x: {}".format(modifier.bin_count_x))
modifier.bin_count_x = 50

print("  bin_count_y: {}".format(modifier.bin_count_y))
modifier.bin_count_y = 80

print("  reduction_operation: {}".format(modifier.reduction_operation))
modifier.reduction_operation = BinAndReduceModifier.Operation.Mean

print("  first_derivative: {}".format(modifier.first_derivative))
modifier.first_derivative = False

print("  reduction_operation: {}".format(modifier.direction))
modifier.direction = BinAndReduceModifier.Direction.Vectors_1_2

print("  property: {}".format(modifier.property))
modifier.property = "Position.X"

node.compute()

print("Output:")

data = modifier.bin_data
print(data)
assert(len(data) == modifier.bin_count_y)
print(modifier.axis_range_x)
print(modifier.axis_range_y)
