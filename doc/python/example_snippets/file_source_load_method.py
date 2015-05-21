from ovito.io import import_file
from ovito.modifiers import ColorCodingModifier

# This creates a new node with an empty modification pipeline:
node = import_file('first_file.dump')

# Populate the pipeline with a modifier:
node.modifiers.append(ColorCodingModifier(property='Potential Energy'))

# Call FileSouce.load() to replace the input data with a different file
# but keep the node's current modification pipeline:
node.source.load('second_file.dump')