# Post build script
import os
import sys

SOURCE_PATH = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..', '..'))
BUILD_PATH = os.path.join(SOURCE_PATH, 'build')

# FIXME move the helper python modules somewher other than the root of the repo
sys.path.append(SOURCE_PATH)

import hifi_utils

#for var in sys.argv:
#    print("{}".format(var))

#for var in os.environ:
#    print("{} = {}".format(var, os.environ[var]))

print("Create ZIP version of installer archive")
hifi_utils.executeSubprocess(['cpack', '-G', 'ZIP'], folder=BUILD_PATH)
