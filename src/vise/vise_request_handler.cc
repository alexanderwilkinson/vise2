#include "vise/vise_request_handler.h"

vise::vise_request_handler *vise::vise_request_handler::vise_request_handler_ = NULL;

vise::vise_request_handler* vise::vise_request_handler::instance() {
  if ( !vise_request_handler_ ) {
    vise_request_handler_ = new vise::vise_request_handler;
  }
  return vise_request_handler_;
}

void vise::vise_request_handler::init(const boost::filesystem::path vise_asset_dir) {
  asset_dir_ = vise_asset_dir;
  BOOST_LOG_TRIVIAL(debug) << "ImageMagick Magick++ quantum depth = " << MAGICKCORE_QUANTUM_DEPTH << flush;
  BOOST_LOG_TRIVIAL(debug) << "vise_request_handler::init() : asset_dir=" << asset_dir_.string() << flush;
}

void vise::vise_request_handler::handle_http_request(const http_request& request, http_response& response) {
  //BOOST_LOG_TRIVIAL(debug) << request.method_ << " [" << request.uri_ << "]";
  std::vector<std::string> uri_components;
  std::map<std::string, std::string> uri_param;
  vise::util::decompose_uri(request.uri_, uri_components, uri_param);
  //vise::util::print_vector("uri_components", uri_components);
  //vise::util::print_map("uri_param", uri_param);

  if ( request.method_ == "POST" ) {
    response.set_status(400);
    return;
  }

  /*
  if ( request.method_ == "POST" ) {
    if ( vise::util::starts_with(request.uri_, "/vise/admin/") ) {
      if ( uri_components.size() < 3 ) {
        response.set_status(400);
        return;
      }
      if ( uri_components.size() != 6 || uri_components.size() != 4) {
        response.set_status(400);
        return;
      }
      if ( uri_components[1] != "vise" || uri_components[2] != "admin" ) {
        response.set_status(400);
        return;
      }

      // POST /vise/admin/_NAME_/_VERSION_/_COMMAND_
      if ( uri_components.size() == 6 ) {
        std::string search_engine_name    = uri_components[3];
        std::string search_engine_version = uri_components[4];
        std::string search_engine_command = uri_components[5];

        if ( vise::util::has_special_char(search_engine_name) ||
             vise::util::has_special_char(search_engine_version) ||
             vise::util::has_special_char(search_engine_command) ) {
          response.set_status(400);
          BOOST_LOG_TRIVIAL(debug) << "search engine uri [" << request.uri_ << "] contains invalid character";
          return;
        }

        std::string search_engine_id = vise::search_engine::get_search_engine_id(search_engine_name, search_engine_version);
        vise::search_engine_manager::instance()->process_cmd(search_engine_id,
                                                             search_engine_command,
                                                             uri_param,
                                                             request.payload_.str(),
                                                             response);
        return;
      }
      response.set_status(400);
      return;
    }
  }
  */

  // For the sake of completeness, vise includes a minimal (and possibly inefficient) file content server.
  // In production environments, the GET requests should be handled by ngnix server.
  if ( request.method_ == "GET" ) {
    /*
      if ( vise::util::has_special_char(request.uri_) ) {
      response.set_status(400);
      BOOST_LOG_TRIVIAL(debug) << "http uri [" << request.uri_ << "] contains invalid character";
      return;
      }
    */

    if ( vise::util::starts_with(request.uri_, "/vise/query/") ) {
      BOOST_LOG_TRIVIAL(debug) << request.method_ << " [" << request.uri_ << "]";
      //std::cout << "\nuri_components.size() = " << uri_components.size() << ", uri_param=" << uri_param.size() << std::flush;
      if ( uri_components.size() != 5 && uri_components.size() != 6 ) {
        response.set_status(404);
        return;
      }

      std::string search_engine_name      = uri_components[3];
      std::string search_engine_version   = uri_components[4];

      /*
        if ( vise::util::has_special_char(search_engine_name) ||
        vise::util::has_special_char(search_engine_version) ||
        vise::util::has_special_char(search_engine_command) ) {
        response.set_status(400);
        BOOST_LOG_TRIVIAL(debug) << "search engine uri [" << request.uri_ << "] contains invalid character";
        return;
        }
      */

      std::string search_engine_id = vise::search_engine::get_search_engine_id(search_engine_name, search_engine_version);
      if ( ! vise::search_engine_manager::instance()->is_search_engine_loaded(search_engine_id) ) {
        vise::search_engine_manager::instance()->load_search_engine(search_engine_id);

        if ( ! vise::search_engine_manager::instance()->is_search_engine_loaded(search_engine_id) ) {
          response.set_status(404);
          BOOST_LOG_TRIVIAL(debug) << "failed to load search engine: " << search_engine_id;
          return;
        }
      }

      // by default, show _filelist
      std::string search_engine_command;
      if ( uri_components.size() == 6 ) {
        search_engine_command = uri_components[5];
      } else {
        // forward request to vise/query/19c_image_match/1/_filelist?from=0&count=1024&show_from=0&show_count=45
        response.set_status(303);
        std::ostringstream forward;
        forward << "/vise/query/" << search_engine_id << "/_filelist?from=0&count=1024&show_from=0&show_count=45";
        response.set_field("Location", forward.str());
        return;
      }

      //std::cout << "\nsearch_engine_command = " << search_engine_command << std::flush;

      //BOOST_LOG_TRIVIAL(debug) << "running [" << search_engine_command << "] on [" << search_engine_id << "]";
      vise::search_engine_manager::instance()->query(search_engine_id,
                                                     search_engine_command,
                                                     uri_param,
                                                     request.payload_.str(),
                                                     response);
      return;
    }

    // in production envrionment, any resources inside "/vise/asset/" uri
    // are static resources that can be served by a more efficient file server
    if ( vise::util::starts_with(request.uri_, "/vise/asset/") ) {
      if ( uri_components.size() != 7 ) {
        response.set_status(404);
        BOOST_LOG_TRIVIAL(debug) << "got " << request.uri_ << ", expected /vise/asset/_NAME_/_VERSION_/{image,thumnail,original}/{filename,file_id}";
        return;
      }

      std::string search_engine_name      = uri_components[3];
      std::string search_engine_version   = uri_components[4];
      std::string asset_type              = uri_components[5];
      std::string asset_name              = uri_components[6];

      std::string search_engine_id = vise::search_engine::get_search_engine_id(search_engine_name, search_engine_version);
      if ( ! vise::search_engine_manager::instance()->is_search_engine_loaded(search_engine_id) ) {
        vise::search_engine_manager::instance()->load_search_engine(search_engine_id);

        if ( ! vise::search_engine_manager::instance()->is_search_engine_loaded(search_engine_id) ) {
          response.set_status(400);
          BOOST_LOG_TRIVIAL(debug) << "failed to load search engine: " << search_engine_id;
          return;
        }
      }

      //BOOST_LOG_TRIVIAL(debug) << "running [" << asset_type << "/" << asset_name << "] on [" << search_engine_id << "]";

      vise::search_engine_manager::instance()->asset(search_engine_id,
                                                     asset_type,
                                                     asset_name,
                                                     uri_param,
                                                     request.payload_.str(),
                                                     response);
      return;
    }

    if ( vise::util::starts_with(request.uri_, "/vise") ) {
      boost::filesystem::path fn = asset_dir_ / request.uri_;
      if ( request.uri_ == "/vise" ||
           request.uri_ == "/vise/"  ||
           request.uri_ == "/vise/index.html") {
        //fn = asset_dir_ / "/vise/home.html";
        response.set_status(400);
        return;
      }

      // @todo: set header fields such that these files are cached
      //response.set_field("Cache-Control", "public;max-age=3600");

      respond_with_static_file( response, fn );
      return;
    }
  }

  // all unhandled cases result in HTTP response 400 (Bad Request)
  response.set_status(400);
  return;
}

void vise::vise_request_handler::respond_with_static_file(http_response& response, boost::filesystem::path fn) {
  std::string file_content;
  bool ok = vise::util::load_file_content(fn, file_content);
  if ( ok ) {
    response.set_payload( file_content );
    response.set_content_type_from_filename( fn.string() );
    //BOOST_LOG_TRIVIAL(debug) << "http response contains file [" << fn.string() << "]";
  } else {
    response.set_status(400);
    BOOST_LOG_TRIVIAL(debug) << "failed to send file in http response [" << fn.string() << "]";
  }
}
