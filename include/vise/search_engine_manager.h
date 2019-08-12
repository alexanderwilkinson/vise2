/** @file   search_engine_manager.h
 *  @brief  A singleton class which handles http requests related to vise
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   18 June 2018
 */

#ifndef _VISE_SEARCH_ENGINE_MANAGER_H_
#define _VISE_SEARCH_ENGINE_MANAGER_H_

#include "vise_server_config.h"    // contains vise version number
#include "vise/util.h"
#include "vise/search_engine.h"
#include "vise/relja_retrival.h"
#include "imreg_sift/imreg_sift.h" // for image registration

#include <iostream>
#include <sstream>
#include <fstream>

// for thread management
#include <thread>
#include <mutex>

// thread
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/process.hpp>

// for filesystem i/o
#include <boost/filesystem.hpp>

// for logging
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

using namespace std;
using namespace Eigen;

// uses C++ singleton design pattern
namespace vise {
class search_engine_manager {

  boost::filesystem::path data_dir_;  // storage for vise internal data
  boost::filesystem::path asset_dir_; // images, thumbnails, etc
  boost::filesystem::path temp_dir_;

  static search_engine_manager* search_engine_manager_;

  // threads management
  boost::thread* search_engine_index_thread_;
  bool search_engine_index_thread_running_;
  std::mutex load_search_engine_mutex_;

  std::string now_search_engine_name_;
  std::string now_search_engine_version_;
  std::map<std::string, std::string> now_search_engine_index_steps_done_;
  std::map<std::string, std::string> now_search_engine_index_steps_count_;
  std::map<std::string, std::string> now_search_engine_index_state_;
  std::vector<std::string> search_engine_index_state_name_list_;
  std::vector<std::string> search_engine_index_state_desc_list_;
  std::map<std::string, vise::search_engine* > search_engine_list_;

  search_engine_manager() { };
  search_engine_manager(const search_engine_manager& sh) { };
  search_engine_manager* operator=(const search_engine_manager &) {
    return 0;
  }

  public:
  static search_engine_manager* instance();

  static std::string RESPONSE_HTML_PAGE_PREFIX;
  static std::string RESPONSE_HTML_PAGE_SUFFIX;

  ~search_engine_manager(void) {
    unload_all_search_engine();
  }

  // must be called before any thread start using search_engine_manager
  void init(const boost::filesystem::path data_dir,
            const boost::filesystem::path asset_dir,
            const boost::filesystem::path temp_dir);

  // POST /vise/admin/_NAME_/_VERSION_/_COMMAND_
  void process_cmd(const std::string search_engine_id,
                   const std::string search_engine_command,
                   const std::map<std::string, std::string> uri_param,
                   const std::string request_body,
                   http_response& response);
  // POST /vise/admin/_COMMAND_
  void admin(const std::string command,
             const std::map<std::string, std::string> uri_param,
             const std::string request_body,
             http_response& response);
  // POST /vise/query/_NAME_/_VERSION_/_COMMAND_
  void query(const std::string search_engine_id,
             const std::string search_engine_command,
             const std::map<std::string, std::string> uri_param,
             const std::string request_body,
             http_response& response);

  // GET /vise/asset/_NAME_/_VERSION/{image,thumbnail,original}/{filename,file_id}
  void asset(const std::string search_engine_id,
             const std::string asset_type,
             const std::string asset_name,
             const std::map<std::string, std::string> uri_param,
             const std::string request_body,
             http_response& response);

  // management
  void load_search_engine(std::string search_engine_id);
  bool unload_search_engine(std::string search_engine_name, std::string search_engine_version);
  bool unload_all_search_engine();
  bool load_search_engine_check(std::string search_engine_name, std::string search_engine_version);

  bool is_search_engine_loaded(std::string search_engine_id);

  // search engine data dir management
  boost::filesystem::path convert_to_unique_filename(boost::filesystem::path filename);
  std::string get_unique_filename(std::string extension="");
  bool search_engine_exists(const std::string search_engine_id);
  bool create_search_engine(const std::string search_engine_id);

  // image i/o
  bool add_image_from_http_payload(const boost::filesystem::path filename,
                                   const std::string& request_body);

  bool index_start(const std::string search_engine_name,
                   const std::string search_engine_version);
  void clear_now_search_engine_index_state(void);
  bool run_shell_command(std::string name,
                         std::string cmd);

  // util
  std::string get_image_uri_prefix(std::string search_engine_id);
  std::string get_query_uri_prefix(std::string search_engine_id);
};
}
#endif
