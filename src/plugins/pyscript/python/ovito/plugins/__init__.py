import builtins
import pkgutil

if hasattr(builtins, "__ovito_plugin_paths"):
    # Load C++ extension modules from the native plugins path when running in the embedded interpreter.
    __path__ += builtins.__ovito_plugin_paths
else:
    # Load C++ extension modules from a directory in sys.path when running in an external interpreter.
    __path__ = pkgutil.extend_path(__path__, __name__)


