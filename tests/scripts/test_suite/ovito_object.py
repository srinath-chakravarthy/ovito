from ovito import *

o1 = ObjectNode()
print(str(o1))
print(repr(o1))
assert(o1 == o1)
o2 = ObjectNode()
assert(o1 != o2)
assert(ovito.dataset.anim == ovito.dataset.anim)