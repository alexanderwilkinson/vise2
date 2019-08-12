/** @file   vise_server.cc
 *  @brief  main entry point for the vise server
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   18 June 2018
 */
#define BOOST_ASIO_ENABLE_BUFFER_DEBUGGING

#define RR_MPI
#include "mpi_queue.h" // required by MPI_INIT_ENV

#include <cstdio>
#include <csignal>

#include <iostream>
#include <string>
#include <thread>

#include <boost/filesystem.hpp>
#include <Magick++.h>            // to transform images

#include "vise_server_config.h"
#include "http_server/http_server.h"
#include "vise/vise_request_handler.h"

// for logging
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#if defined(_WIN32) || defined(WIN32)
  #include <windows.h>
  #include <ShellAPI.h>
#endif

int main(int argc, char** argv) {
  MPI_INIT_ENV
  std::cout << VISE_SERVER_NAME << " "
            << VISE_SERVER_VERSION_MAJOR << "."
            << VISE_SERVER_VERSION_MINOR << "."
            << VISE_SERVER_VERSION_PATCH
            << " [" << VISE_SERVER_URL << "]";

  std::cout << "\nAuthor: "
            << VISE_SERVER_AUTHOR_NAME << " <"
            << VISE_SERVER_AUTHOR_EMAIL << ">"
//            << "\nRelease: " << VISE_SERVER_CURRENT_RELEASE_DATE
            << std::endl;

  if ( argc != 6 && argc != 8 && argc != 9 && argc != 1) {
    std::cout << "\nUsage: " << argv[0]
              << " hostname port thread_count vise_asset_dir [vise_data_dir | search_engine_{data,asset,temp}_dir]] [PRELOAD_SEARCH_ENGINE]\n"
              << "\ne.g.: ./" << argv[0] << " 0.0.0.0 9973 4 /home/tlm/dev/vise/asset"
              << "\n      ./" << argv[0] << " 0.0.0.0 9973 4 /home/tlm/dev/vise/asset /ssd/data/vise"
              << "\n      ./" << argv[0] << " 0.0.0.0 9973 4 /home/tlm/dev/vise/asset /ssd/vise/data /data/vise/asset /tmp/vise"
              << "\n      ./" << argv[0] << " 0.0.0.0 9973 4 /home/tlm/dev/vise/asset /ssd/vise/data /data/vise/asset /tmp/vise ox5k/1"
              << std::endl;
    return 0;
  }

  // default values
  std::string address("0.0.0.0");
  std::string port("9973");
  boost::filesystem::path exec_dir( argv[0] );

  Magick::InitializeMagick(exec_dir.parent_path().string().c_str());
  //std::cout << "\nMagick::InitializeMagick = " << exec_dir.parent_path().string().c_str() << std::endl;

  boost::filesystem::path vise_asset_dir( exec_dir.parent_path() / "asset");
  unsigned int thread_pool_size = 4;

  boost::filesystem::path vise_data_dir( boost::filesystem::temp_directory_path() / "vise" );
  boost::filesystem::path se_data_dir  = vise_data_dir / "data";
  boost::filesystem::path se_asset_dir = vise_data_dir / "asset";
  boost::filesystem::path se_temp_dir  = vise_data_dir / "temp";
  std::string preload_search_engine    = "";
  if ( argc >= 5 ) {
    // vise_data_dir not provided, use /temp
    address = argv[1];
    port    = argv[2];

    std::stringstream s;
    s.clear(); s.str(argv[3]);
    s >> thread_pool_size;

    vise_asset_dir = boost::filesystem::path(argv[4]);

    if ( argc == 6 ) {
      vise_data_dir = boost::filesystem::path( argv[5] );
    }
    if ( argc == 8 || argc == 9) {
      se_data_dir  = boost::filesystem::path( argv[5] );
      se_asset_dir = boost::filesystem::path( argv[6] );
      se_temp_dir  = boost::filesystem::path( argv[7] );
      vise_data_dir = boost::filesystem::path( argv[5] );
      if ( argc == 9 ) {
        preload_search_engine = std::string(argv[8]);
      }
    }
  } else {
    cout << "\nUsing default settings ...";
  }

  if ( !boost::filesystem::exists(vise_data_dir) ) {
    boost::filesystem::create_directories(vise_data_dir);
  } else {
    // cleanup
    //boost::filesystem::remove_all( data_dir );
    //boost::filesystem::create_directories(data_dir);
  }

  if ( !boost::filesystem::exists(se_data_dir) ) {
    boost::filesystem::create_directories( se_data_dir );
  }
  if ( !boost::filesystem::exists(se_asset_dir) ) {
    boost::filesystem::create_directories( se_asset_dir );
  }
  if ( !boost::filesystem::exists(se_temp_dir) ) {
    boost::filesystem::create_directories( se_temp_dir );
  }

  /*
  // initialize logger
  boost::filesystem::path log_fn = se_temp_dir / boost::filesystem::unique_path("vise-%%%%%%.log");
  boost::log::add_file_log(log_fn.string());
  boost::log::trivial::add_common_attributes();
  boost::log::trivial::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
  */

  // this is critical to avoid race conditions for vise_request_handler::instance()
  // initialize http request handler
  vise::vise_request_handler::instance()->init(vise_asset_dir);
  // initialize search engine manager
  vise::search_engine_manager::instance()->init(se_data_dir, se_asset_dir, se_temp_dir);

  if ( preload_search_engine != "" ) {
    vise::search_engine_manager::instance()->load_search_engine( preload_search_engine );
  }

  http_server server(address, port, thread_pool_size);
  std::cout << "\n\nNotes:";
  std::cout << "\n  - To use the application, visit http://localhost:" << port << "/vise/ in a web browser" << std::endl;

  server.start();
  return 0;
}
