from ovito.io import import_file
from ovito.modifiers import CoordinationNumberModifier
import numpy

# Load a particle dataset, apply modifier, and evaluate data pipeline.
node = import_file("simulation.dump")
modifier = CoordinationNumberModifier(cutoff = 5.0, number_of_bins = 200)
node.modifiers.append(modifier)
node.compute()

# Export the computed RDF data to a text file.
numpy.savetxt("output_rdf.txt", modifier.rdf)
