/** @file   test_search_engine_manager.cc
 *  @brief  tests for search_engine_manager.cc
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   20 June 2018
 */

#define RR_MPI

#include <iostream>
#include <string>
#include <map>
#include <thread>

#define BOOST_LOG_DYN_LINK 1
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include "vise/search_engine_manager.h"
#include "http_server/http_response.h"

#include "mpi_queue.h"
using namespace std;

bool test_search_engine_manager_add_image() {
  http_response response;
  map<string, string> uri_param;
  string search_engine_id = "ox5k/1";
  string search_engine_command = "add_file?filename=\"all_souls_00001.jpg\"";
  string payload = "base64 image data";
  vise::search_engine_manager::instance()->process_cmd(search_engine_id, search_engine_command, uri_param, payload, response);
}

bool test_search_engine_manager_load_search_engine() {
  string search_engine_id = "ox5k/1";
  string search_engine_command = "query";

  vise::search_engine_manager::instance()->load_search_engine(search_engine_id);

  std::map<std::string, std::string> uri_param;
  uri_param[ "file_id" ] = "2";
  uri_param[ "region" ] = "156,228,316,502";
  uri_param[ "from" ] = "0";
  uri_param[ "to" ] = "30";
  uri_param[ "score_threshold" ] = "0.0";
  std::string body;
  http_response response;

  vise::search_engine_manager::instance()->query(search_engine_id,
                                                 search_engine_command,
                                                 uri_param,
                                                 body,
                                                 response);
  //search_engine_manager::instance()->unload_all_search_engine();
}

bool test_search_engine_manager_query_search_engine() {
  string search_engine_id = "ox5k/1";
  string search_engine_command = "query";

  std::map<std::string, std::string> uri_param;
  uri_param[ "file_id" ] = "2";
  uri_param[ "region" ] = "156,228,316,502";
  uri_param[ "from" ] = "0";
  uri_param[ "result_count" ] = "5000";
  uri_param[ "score_threshold" ] = "0.0";
  std::string body;
  http_response response;

  std::cout << "\nRunning query ... " << std::flush;
  vise::search_engine_manager::instance()->query(search_engine_id,
                                                 search_engine_command,
                                                 uri_param,
                                                 body,
                                                 response);
  //search_engine_manager::instance()->unload_all_search_engine();
}


int main(int argc, char** argv) {
  MPI_INIT_ENV
  // boost::filesystem::path data_dir( boost::filesystem::temp_directory_path() / "vise" );
  // boost::filesystem::path search_engine_data_dir = data_dir / "repo";
  // search_engine_manager::instance()->init(search_engine_data_dir);
  // test_search_engine_manager_add_image();

  boost::filesystem::path data_dir( "/home/tlm/mydata/vise" );
  boost::filesystem::path search_engine_data_dir  = data_dir / "data";
  boost::filesystem::path search_engine_asset_dir = data_dir / "asset";
  boost::filesystem::path search_engine_temp_dir  = data_dir / "temp";
  vise::search_engine_manager::instance()->init(search_engine_data_dir,
                                                search_engine_asset_dir,
                                                search_engine_temp_dir);
  test_search_engine_manager_load_search_engine();

  std::thread a( test_search_engine_manager_query_search_engine);
  std::thread b( test_search_engine_manager_query_search_engine);

  std::thread d( test_search_engine_manager_query_search_engine);
  std::thread e( test_search_engine_manager_query_search_engine);

  a.join();
  b.join();
  delete vise::search_engine_manager::instance(); // leads to memory leak if not invoked
}
