import ovito

o1 = ovito.ObjectNode()
print(str(o1))
print(repr(o1))
assert(o1 == o1)
o2 = ovito.ObjectNode()
assert(o1 != o2)
assert(ovito.dataset.anim == ovito.dataset.anim)