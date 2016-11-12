from ovito import *

assert(len(ovito.version) == 3)
print(ovito.version_string)
print(ovito.gui_mode)
print(ovito.headless_mode)

# Progress display
progress = ovito.get_progress_display()
if progress is not None:
    print(progress.canceled)