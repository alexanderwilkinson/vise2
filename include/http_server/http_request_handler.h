/** @file   http_request_handler.h
 *  @brief  A prototype for classes that handle http requests
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   27 Nov 2017
 */

#ifndef _HTTP_REQUEST_HANDLER_H_
#define _HTTP_REQUEST_HANDLER_H_

#include <iostream>

#include <boost/filesystem.hpp>

#include "http_server/http_request.h"
#include "http_server/http_response.h"

using namespace std;

// uses C++ singleton design pattern
class http_request_handler {
  public:
  static http_request_handler* instance();
  virtual void init();
  virtual void handle_http_request(const http_request& request, http_response& response);

  private:
  http_request_handler() { };
  http_request_handler(const http_request_handler& sh) { };
  http_request_handler* operator=(const http_request_handler &) {
    return 0;
  }

  static http_request_handler* http_request_handler_;
};
#endif

