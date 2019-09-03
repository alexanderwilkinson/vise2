
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_library( util util.cc)
target_link_libraries( util ${Boost_LIBRARIES} )

add_library( relja_retrival search_engine.cc relja_retrival.cc )

target_link_libraries(  relja_retrival 
                        dataset_v2 
                        proto_db
                        proto_db_file
                        proto_index
                        slow_construction
                        feat_standard
                        clst_centres
                        hamming
                        hamming_embedder
                        spatial_api
                        spatial_verif_v2
                        tfidf_v2
                        mq_filter_outliers
                        ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} ${fastann_LIBRARIES})

add_library( search_engine_manager search_engine_manager.cc)
target_link_libraries( search_engine_manager relja_retrival http_request http_response util imreg_sift ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} )

add_library( vise_request_handler vise_request_handler.cc)
target_link_libraries( vise_request_handler http_request http_response util search_engine_manager ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} )

add_executable( mpi_search mpi_search.cc )
set_property(SOURCE mpi_search.cc APPEND PROPERTY COMPILE_FLAGS "-g")
target_link_libraries( mpi_search
  search_engine_manager
  http_request
  http_response
  ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} ${fastann_LIBRARIES})

add_executable( batch_search batch_search.cc )
set_property(SOURCE batch_search.cc APPEND PROPERTY COMPILE_FLAGS "-g")
target_link_libraries( batch_search
  search_engine_manager
  http_request
  http_response
  ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} ${fastann_LIBRARIES})

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  # compile unit tests
#  add_executable( test_search_engine_manager test_search_engine_manager.cc )
#  target_link_libraries( test_search_engine_manager search_engine_manager http_request http_response util ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} -lm -lpthread)

  add_executable( test_search_engine_manager test_search_engine_manager.cc )
  target_link_libraries( test_search_engine_manager 
  search_engine_manager 
  http_request 
  http_response 
  ${ImageMagick_LIBRARIES} ${Boost_LIBRARIES} ${fastann_LIBRARIES})
endif()
