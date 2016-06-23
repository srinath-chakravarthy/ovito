from ovito.io import import_file

node = import_file("simulation.dump")
node.add_to_scene()
node.source.cell.display.line_width = 1.3
