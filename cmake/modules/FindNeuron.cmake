#
#  FindNeuron.cmake
#
#  Try to find the Perception Neuron SDK
#
#  You must provide a NEURON_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  NEURON_FOUND - system found Neuron SDK
#  NEURON_INCLUDE_DIRS - the Neuron SDK include directory
#  NEURON_LIBRARIES - Link this to use Neuron
#
#  Created on 12/21/2015 by Anthony J. Thibault
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

include(SelectLibraryConfigurations)
select_library_configurations(NEURON)

set(NEURON_REQUIREMENTS NEURON_INCLUDE_DIRS NEURON_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Neuron DEFAULT_MSG NEURON_INCLUDE_DIRS NEURON_LIBRARIES)
mark_as_advanced(NEURON_LIBRARIES NEURON_INCLUDE_DIRS NEURON_SEARCH_DIRS)

