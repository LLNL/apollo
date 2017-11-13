import os
import re
import sys

def attr_setdefault(obj, name, value):
    """Like dict.setdefault, but for objects."""
    if not hasattr(obj, name):
        setattr(obj, name, value)
    return getattr(obj, name)


# Patterns to ignore in the commands directory when looking for commands.
ignore_files = r'^\.|^__init__.py$|^#'

SETUP_PARSER = "setup_parser"
DESCRIPTION  = "description"

command_path = os.path.dirname(os.path.realpath(__file__))

commands = []
for file in os.listdir(command_path):
    if file.endswith(".py") and not re.search(ignore_files, file):
        cmd = re.sub(r'.py$', '', file)
        commands.append(cmd)
commands.sort()


def get_cmd_function_name(name):
    return name.replace("-", "_")


def get_module(name):
    """Imports the module for a particular command name and returns it."""
    module_name = "%s.%s" % (__name__, name)
    module = __import__(
        module_name, fromlist=[name, SETUP_PARSER, DESCRIPTION],
        level=0)

    attr_setdefault(module, SETUP_PARSER, lambda *args: None) # null-op
    attr_setdefault(module, DESCRIPTION, "")

    fn_name = get_cmd_function_name(name)
    if not hasattr(module, fn_name):
        sys.stderr.write("Command module %s (%s) must define function '%s'."
                % (module.__name__, module.__file__, fn_name))

    return module


def get_command(name):
    """Imports the command's function from a module and returns it."""
    return getattr(get_module(name), get_cmd_function_name(name))
