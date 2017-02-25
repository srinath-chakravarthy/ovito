import sys

# This is the ovito.plugins Python package. It contains the native C++ plugin libraries of OVITO.

# Our C++ extension modules are, however, located in a different directory of the OVITO installation.
# For the time being, we use hardcoded relative paths to find them.

# Platform-dependent paths where this Python module is located:

  # Linux:   lib/ovito/plugins/python/ovito/plugins/
  # Windows: plugins/python/ovito/plugins/
  # macOS:   Ovito.app/Contents/Resources/python/ovito/plugins/

# Platform-dependent paths where the native C++ plugins are located:

  # Linux:   lib/ovito/plugins/
  # Windows: plugins/
  # macOS:   Ovito.app/Contents/PlugIns/

if sys.platform.startswith('darwin'):  # macOS
    __path__[0] += "/../../../../PlugIns"
else:
    __path__[0] += "/../../.."
