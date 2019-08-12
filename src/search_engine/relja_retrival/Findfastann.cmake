#
# Try to find the fastann library and include path.
# Once done this will define
#
# FASTANN_FOUND
# fastann_INCLUDE_DIRS
# fastann_LIBRARIES
# 

find_path(fastann_INCLUDE_DIR
  NAMES fastann.hpp
  PATH_SUFFIXES fastann
  DOC "The directory where fastann.hpp resides")

find_library(fastann_LIBRARY
  NAMES fastann
  DOC "The fastann library")

# handle the QUIETLY and REQUIRED arguments and set FASTANN_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(fastann DEFAULT_MSG fastann_LIBRARY fastann_INCLUDE_DIR)

if (FASTANN_FOUND)
  set(fastann_LIBRARIES ${fastann_LIBRARY})
  set(fastann_INCLUDE_DIRS ${fastann_INCLUDE_DIR})
endif(FASTANN_FOUND)

mark_as_advanced(
  fastann_INCLUDE_DIR
  fastann_LIBRARY)
 
