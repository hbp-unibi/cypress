# - Try to find Cypress
# Once done this will define
#  CYPRESS_FOUND - System has CYPRESS
#  CYPRESS_INCLUDE_DIRS - The CYPRESS include directories
#  CYPRESS_LIBRARIES - The libraries needed to use CYPRESS

find_path(CYPRESS_INCLUDE_DIR cypress/cypress.hpp PATH_SUFFIXES cypress)
find_library(CYPRESS_LIBRARY NAMES cypress libcypress)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cypress DEFAULT_MSG
                                  CYPRESS_LIBRARY CYPRESS_INCLUDE_DIR)

mark_as_advanced(CYPRESS_INCLUDE_DIR CYPRESS_LIBRARY)
SET(PYBIND11_PYTHON_VERSION "2.7")
find_package(pybind11 REQUIRED)

set(ONLY_CYPRESS ${CYPRESS_LIBRARY})
set(CYPRESS_LIBRARY ${CYPRESS_LIBRARY} 
            pybind11::embed
            dl
            -pthread)
            
set(CYPRESS_INCLUDE_DIRS ${CYPRESS_INCLUDE_DIR} 
        ${PYTHON_NUMPY_INCLUDE_DIR}
        ${PYTHON_INCLUDE_DIR})


add_library(cypress STATIC IMPORTED)
set_target_properties(cypress PROPERTIES IMPORTED_LOCATION ${ONLY_CYPRESS})
