import sys
from ovito.io import *
from ovito.modifiers import *

node = None
for file in sys.argv[1:]:

    if not node:
        # Import the first file using import_file().
        # This creates the ObjectNode and sets up the modification pipeline.
        node = import_file(file)
        # Insert a modifier into the pipeline.
        cna = CommonNeighborAnalysisModifier()
        node.modifiers.append(cna)
    else:
        # To load subsequent files, call the load() function of the FileSource.
        node.source.load(file)

    # Evaluate pipeline and wait until the analysis results are available.
    node.compute()
    print("Structure %s contains %i FCC atoms." % (file, cna.counts["FCC"]))