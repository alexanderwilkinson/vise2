# for protobufs
# protobuf compiles *.pb.cc and *.pb.h headers in the cmake build tree
# therefore, we need to include the corresponding folders in the build tree
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/util )
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/v2/dataset )
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/v2/embedding )
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/v2/evaluation )
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/v2/indexing )
include_directories( ${CMAKE_BINARY_DIR}/src/search_engine/relja_retrival/v2/retrieval )

# includes for search_engine/relja_retrival
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/dataset")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/util")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/dataset")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/embedding")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/evaluation")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/indexing")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/v2/retrieval")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/retrieval")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/represent")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/matching/det_ransac")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/indexing")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/preprocessing")
include_directories("${CMAKE_SOURCE_DIR}/src/search_engine/relja_retrival/compression")

add_subdirectory( search_engine/relja_retrival )
add_subdirectory( http_server )
add_subdirectory( vise )
add_subdirectory( imreg_sift )

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_executable( vise_server vise_server.cc )
  target_link_libraries( vise_server http_server vise_request_handler search_engine_manager ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} )
endif()

