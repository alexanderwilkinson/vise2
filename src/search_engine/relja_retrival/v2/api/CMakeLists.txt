if (cREGISTER)
    set(REGISTER_LIB "register_images")
endif (cREGISTER)

add_executable( api_v2 api_v2.cpp )
target_link_libraries( api_v2
    clst_centres
    dataset_v2
    feat_standard
    hamming
    hamming_embedder
    mq_filter_outliers
    proto_db
    proto_db_file
    proto_index
    slow_construction
    spatial_api
    spatial_verif_v2
    tfidf_v2
    ${Boost_LIBRARIES}
    ${fastann_LIBRARIES} )
