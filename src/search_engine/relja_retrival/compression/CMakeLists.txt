#add_subdirectory(tests)

add_library( char_streams char_streams.cpp )
target_link_libraries( char_streams )

add_library( product_quant product_quant.cpp )
target_link_libraries( product_quant clst_centres char_streams ${fastann_LIBRARIES} )
