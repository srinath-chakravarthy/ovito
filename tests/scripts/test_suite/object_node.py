from ovito.io import *
from ovito.modifiers import *

node = import_file("../../files/CFG/shear.void.120.cfg")

assert(len(node.modifiers) == 0)
mod1 = AssignColorModifier()
node.modifiers.append(mod1)
assert(len(node.modifiers) == 1)
assert(node.modifiers[0] == mod1)
mod2 = AssignColorModifier()
node.modifiers.append(mod2)
assert(len(node.modifiers) == 2)
assert(node.modifiers[0] == mod1)
assert(node.modifiers[1] == mod2)
mod3 = AssignColorModifier()
node.modifiers.insert(1, mod3)
assert(len(node.modifiers) == 3)
assert(node.modifiers[1] == mod3)
del node.modifiers[0]
assert(len(node.modifiers) == 2)
assert(node.modifiers[0] == mod3)
assert(node.modifiers[1] == mod2)
del node.modifiers[-2]
del node.modifiers[-1]
assert(len(node.modifiers) == 0)
