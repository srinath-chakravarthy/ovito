""" 
    This OVITO script writes the number of bonds at each timestep to a text file.
    
    You should run this script from within the graphical user interface
    using the Scripting->Run Script File menu option.
"""
from ovito import *
from ovito.data import *

# Open output file for writing:
fout = open("outputfile.txt", "w")

for frame in range(dataset.anim.first_frame, dataset.anim.last_frame+1):
	dataset.anim.current_frame = frame
	data = dataset.selected_node.compute()
	num_bonds = len(data.bonds.array) // 2   # Divide by 2 because each bond is represented by two half-bonds.
	print(frame, num_bonds, file = fout)

fout.close()
