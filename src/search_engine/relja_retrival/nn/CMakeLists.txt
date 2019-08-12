
add_library( nn_compressed nn_compressed.cpp )
target_link_libraries( nn_compressed )

add_library( coarse_residual coarse_residual.cpp )
target_link_libraries( coarse_residual nn_compressed index_with_data_file clst_centres ${fastann_LIBRARIES} )
