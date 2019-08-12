##
## VGG Image Search Engine (VISE)
## vise_server
##
## Author: Abhishek Dutta <adutta@robots.ox.ac.uk>
## 18 June 2018
##

cmake_minimum_required( VERSION 3.10 )
project( vise_server )

set( PROJECT_INCLUDE_DIR "include" )
set( PROJECT_SOURCE_DIR "src" )
set( PROJECT_BINARY_DIR "bin" )
set( PROJECT_TEST_DIR "test" )
set( PROJECT_STATIC_DIR "static" )

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

## export version number in configure header file
set (VISE_SERVER_NAME "vise_server")
set (VISE_SERVER_VERSION_MAJOR "2")
set (VISE_SERVER_VERSION_MINOR "0")
set (VISE_SERVER_VERSION_PATCH "0-beta1")
set (VISE_SERVER_URL "http://www.robots.ox.ac.uk/~vgg/software/vise/")
set (VISE_SERVER_AUTHOR_NAME "Abhishek Dutta")
set (VISE_SERVER_AUTHOR_EMAIL "adutta@robots.ox.ac.uk")
set (VISE_SERVER_FIRST_RELEASE_DATE "25 Sep. 2017")
set (VISE_SERVER_CURRENT_RELEASE_DATE "18 Jun. 2018")
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_SOURCE_DIR}/vise_server_config.h.in" 
  "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_INCLUDE_DIR}/vise_server_config.h"
  )

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set( BUILD_SHARED_LIBS "OFF" )

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  message("*** Debug Mode: test code will be compiled and built. ***")
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	set(CMAKE_CXX_FLAGS "-fpermissive")
endif()

## MPI
find_package(MPI REQUIRED)
if (MPI_FOUND)
    include_directories(SYSTEM ${MPI_INCLUDE_PATH})
else (MPI_FOUND)
    message(SEND_ERROR "This application cannot compile without MPI")
endif (MPI_FOUND)

## Boost
find_package(Boost 1.64.0 COMPONENTS mpi filesystem system thread atomic log serialization REQUIRED)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set( Boost_USE_MULTITHREADED ON )
  set( Boost_USE_DEBUG_RUNTIME OFF )
  set( Boost_USE_STATIC_RUNTIME OFF )
  set( Boost_USE_STATIC_LIBS OFF )
  set( BOOST_ALL_DYN_LINK ON)
  set( Boost_DEBUG OFF )
  add_definitions(-DBOOST_ALL_NO_LIB)
  add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
endif()
link_directories( ${Boost_LIBRARY_DIRS} )
include_directories( ${Boost_INCLUDE_DIRS} )

## ImageMagick
ADD_DEFINITIONS(-DMAGICKCORE_HDRI_ENABLE=0)
ADD_DEFINITIONS(-DMAGICKCORE_QUANTUM_DEPTH=8)
find_package( ImageMagick COMPONENTS Magick++ REQUIRED )
# @todo: on alienware, find_package() points to "/usr/lib/x86_64-linux-gnu/libMagick++-6.Q16.so"
# and does not detect the library available at "/home/tlm/deps/imcomp/lib/lib/libMagick++-6.Q8.so"
#set(ImageMagick_LIBRARIES "/home/tlm/deps/vise/lib/lib/libMagick++-6.Q8.so")
include_directories( ${ImageMagick_INCLUDE_DIRS} )

# Eigen
# requires EIGEN_DIR to be set to the location where Eigen3 is built using cmake
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
include_directories( ${EIGEN3_INCLUDE_DIR} )

## Protocol Buffer
## required by "search_engine/relja_retrival"
include(FindProtobuf)
find_package( Protobuf REQUIRED )
include_directories( ${PROTOBUF_INCLUDE_DIR} )

## James's FASTANN (download from robots/software)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/")
find_package(fastann REQUIRED)
include_directories( ${fastann_INCLUDE_DIRS} )

# VLFEAT
# VLFEAT_LIB must be defined during cmake
# e.g. cmake -DVLFEAT_LIB=/data/mybin/vlfeat/vlfeat-0.9.20/bin/glnxa64/libvl.so -DVLFEAT_INCLUDE_DIR=/data/mybin/vlfeat/vlfeat-0.9.20/vl
# @todo: get rid of absolute path
include_directories( ${VLFEAT_INCLUDE_DIR})

message("Boost_LIBRARIES: " ${Boost_LIBRARIES})
message("Boost_INCLUDE_DIRS: " ${Boost_INCLUDE_DIRS})
message("ImageMagick_LIBRARIES: " ${ImageMagick_LIBRARIES} )
message("ImageMagick_INCLUDE_DIRS: " ${ImageMagick_INCLUDE_DIRS} )
message("EIGEN3_INCLUDE_DIR: " ${EIGEN3_INCLUDE_DIR} )
message("PROTOBUF_INCLUDE_DIR: " ${PROTOBUF_INCLUDE_DIR} )
message("Protobuf_LIBRARIES: " ${Protobuf_LIBRARIES} )
message("fastann_INCLUDE_DIRS: " ${fastann_INCLUDE_DIRS} )
message("VLFEAT_LIB: " ${VLFEAT_LIB} )
message("VLFEAT_INCLUDE_DIR: " ${VLFEAT_INCLUDE_DIR} )

include_directories("${PROJECT_INCLUDE_DIR}")
add_subdirectory("${PROJECT_SOURCE_DIR}" )
#add_subdirectory("${PROJECT_TEST_DIR}" )

