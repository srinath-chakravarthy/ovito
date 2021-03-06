""" 
    This OVITO script writes the number of vacancies and interstitials identified by a
    Wigner-Seitz analysis modifier at each timestep to a text file.
    
    You should run this script from within the graphical user interface
    using the Scripting->Run Script File menu option.
"""
from ovito import *
from ovito.data import *
from ovito.modifiers import *

# Find the WS modifier in the pipeline:
ws_mod = None
for modifier in dataset.selected_node.modifiers:
    if isinstance(modifier, WignerSeitzAnalysisModifier):
        ws_mod = modifier
if ws_mod is None:
    raise RuntimeError("No Wigner-Seitz modifier found in the modification pipeline.")

# Open output file for writing:
fout = open("outputfile.txt", "w")

for frame in range(dataset.anim.first_frame, dataset.anim.last_frame+1):
    dataset.anim.current_frame = frame
    dataset.selected_node.compute()
    num_vacancies = ws_mod.vacancy_count
    num_interstitials = ws_mod.interstitial_count
    print(frame, num_vacancies, num_interstitials, file = fout)

fout.close()
