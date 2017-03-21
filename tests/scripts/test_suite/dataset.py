import ovito

print(ovito.dataset)
assert(isinstance(ovito.dataset, ovito.DataSet))
assert(ovito.dataset == ovito.dataset.anim.dataset)
assert(ovito.dataset.anim.num_dependents >= 1)

scene_nodes = ovito.dataset.scene_nodes
assert(len(scene_nodes) == 0)
assert(ovito.dataset.selected_node is None)

o1 = ovito.ObjectNode()
o1.add_to_scene()
assert(len(scene_nodes) == 1)
assert(o1 in scene_nodes)
assert(scene_nodes[0] == o1)
assert(ovito.dataset.selected_node == o1)

o2 = ovito.ObjectNode()
scene_nodes.append(o2)
assert(len(scene_nodes) == 2)
assert(scene_nodes[1] == o2)
ovito.dataset.selected_node = o2

o1.remove_from_scene()
assert(len(scene_nodes) == 1)
assert(scene_nodes[0] == o2)

del scene_nodes[0]
assert(len(scene_nodes) == 0)
