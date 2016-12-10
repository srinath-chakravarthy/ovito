from ovito import *

assert(len(ovito.version) == 3)
print("version_string=", ovito.version_string)
print("gui_mode=", ovito.gui_mode)
print("headless_mode=", ovito.headless_mode)

# Progress display
progress = ovito.get_progress_display()
if progress is not None:
    print(progress.canceled)