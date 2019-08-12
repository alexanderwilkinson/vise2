add_library( http_request STATIC http_request.cc)
add_library( http_response STATIC http_response.cc)

add_library( http_server STATIC http_server.cc connection.cc)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_libraries( http_server http_request http_response vise_request_handler ${Boost_LIBRARIES} pthread m)
else()
  target_link_libraries( http_server http_request http_response vise_request_handler ${Boost_LIBRARIES})
endif()

