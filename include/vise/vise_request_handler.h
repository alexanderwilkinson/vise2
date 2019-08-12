/** @file   vise_request_handler.h
 *  @brief  A singleton class which handles http requests related to vise
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   18 June 2018
 */

#ifndef _VISE_REQUEST_HANDLER_H_
#define _VISE_REQUEST_HANDLER_H_

#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/filesystem.hpp>

#define BOOST_LOG_DYN_LINK 1
#include <boost/log/trivial.hpp>

// to generate uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <Magick++.h>            // to transform images

#include <eigen3/Eigen/Dense>

#include "http_server/http_request.h"
#include "http_server/http_response.h"

#include "vise/util.h"
#include "vise/search_engine_manager.h"

using namespace std;
using namespace Eigen;

namespace vise {
// uses C++ singleton design pattern
class vise_request_handler {
  boost::filesystem::path data_dir_;  // storage for vise internal data, search engine repo.
  boost::filesystem::path asset_dir_; // location of ui, logo, etc.

  vise_request_handler() { };
  vise_request_handler(const vise_request_handler& sh) { };
  vise_request_handler* operator=(const vise_request_handler &) {
    return 0;
  }

  static vise_request_handler* vise_request_handler_;

  public:
  static vise_request_handler* instance();

  void init(const boost::filesystem::path vise_asset_dir);
  void respond_with_static_file(http_response& response, boost::filesystem::path fn);

  // request handler endpoints: see docs/vise_server_api.org
  void handle_http_request(const http_request& request, http_response& response);
};
}
#endif
