#include "vise/search_engine_manager.h"

vise::search_engine_manager *vise::search_engine_manager::search_engine_manager_ = NULL;

// the HTML page header of every HTML response
std::string vise::search_engine_manager::RESPONSE_HTML_PAGE_PREFIX = R"HTML(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><title>VISE: Search Result</title><link rel="shortcut icon" type="image/x-icon" href="/vise/images/favicon.ico"/><link rel="stylesheet" type="text/css" href="/vise/vise.css" /></head>)HTML";

std::string vise::search_engine_manager::RESPONSE_HTML_PAGE_SUFFIX = R"HTML(</head>)HTML";

vise::search_engine_manager* vise::search_engine_manager::instance() {
  if ( !search_engine_manager_ ) {
    search_engine_manager_ = new vise::search_engine_manager;
  }
  return search_engine_manager_;
}

void vise::search_engine_manager::init(const boost::filesystem::path data_dir,
                                       const boost::filesystem::path asset_dir,
                                       const boost::filesystem::path temp_dir) {
  data_dir_  = data_dir;
  asset_dir_ = asset_dir;
  temp_dir_  = temp_dir;

  search_engine_index_thread_running_ = false;
  now_search_engine_index_state_.clear();

  search_engine_index_state_name_list_.clear();
  search_engine_index_state_name_list_.push_back("relja_retrival:trainDescs");
  search_engine_index_state_name_list_.push_back("relja_retrival:cluster");
  search_engine_index_state_name_list_.push_back("relja_retrival:trainAssign");
  search_engine_index_state_name_list_.push_back("relja_retrival:trainHamm");
  search_engine_index_state_name_list_.push_back("relja_retrival:index");
  search_engine_index_state_desc_list_.clear();
  search_engine_index_state_desc_list_.push_back("Computing image descriptors");
  search_engine_index_state_desc_list_.push_back("Clustering descriptors");
  search_engine_index_state_desc_list_.push_back("Assigning descriptors");
  search_engine_index_state_desc_list_.push_back("Computing embeddings");
  search_engine_index_state_desc_list_.push_back("Indexing");

  BOOST_LOG_TRIVIAL(debug) << "search_engine_manager::init() : data_dir=" << data_dir_.string() << flush;
  BOOST_LOG_TRIVIAL(debug) << "search_engine_manager::init() : asset_dir=" << asset_dir_.string() << flush;
  BOOST_LOG_TRIVIAL(debug) << "search_engine_manager::init() : temp_dir=" << temp_dir_.string() << flush;
}

void vise::search_engine_manager::process_cmd(const std::string search_engine_id,
                                              const std::string search_engine_command,
                                              const std::map<std::string, std::string> uri_param,
                                              const std::string request_body,
                                              http_response& response) {
  if ( search_engine_command != "index_status" ) {
    BOOST_LOG_TRIVIAL(debug) << "processing search engine command: "
                             << search_engine_command << " on "
                             << search_engine_id << " : payload="
			     << request_body.size() << " bytes";
  }
  /*
  if ( search_engine_command == "init" ) {
    if ( search_engine_exists(search_engine_id) ) {
      response.set_status(400);
      response.set_field("Content-Type", "application/json");
      response.set_payload( "{\"error\":\"search engine already exists\"}" );
    } else {
      if ( request_body.length() ) {
        create_search_engine(search_engine_id);
      } else {
        create_search_engine(search_engine_id);
      }
      response.set_status(200);
      response.set_field("Content-Type", "application/json");
      std::ostringstream s;
      s << "{\"ok\":\"search engine created\", "
        << "\"search_engine_id\":\"" << search_engine_id << "\"}";

      response.set_payload( s.str() );
    }
    return;
  }

  if ( search_engine_command == "index_start" ) {
    if ( search_engine_index_thread_running_ ) {
      std::ostringstream s;
      s << "{\"error\":\"indexing ongoing for " << now_search_engine_name_
        << ":" << now_search_engine_version_ << "\"}";
      response.set_status(400);
      response.set_field("Content-Type", "application/json");
      response.set_payload( s.str() );
      return;
    }

    try {
      search_engine_index_thread_ = new boost::thread( boost::bind( &search_engine_manager::index_start, this, search_engine_id ) );
      response.set_status(200);
      response.set_field("Content-Type", "application/json");
      response.set_payload("{\"ok\":\"indexing started\"}");
    } catch( std::exception &e ) {
      std::ostringstream s;
      s << "{\"error\":\"" << e.what() << "\"}";
      response.set_status(400);
      response.set_field("Content-Type", "application/json");
      response.set_payload( s.str() );
    }
    return;
  }

  if ( search_engine_command == "index_status" ) {
    if ( now_search_engine_index_state_.size() ) {
      std::ostringstream s;
      std::string name, desc;
      s << "[";
      for ( std::size_t i = 0; i < search_engine_index_state_name_list_.size(); ++i ) {
        name = search_engine_index_state_name_list_.at(i);
        desc = search_engine_index_state_desc_list_.at(i);
        if ( i != 0 ) {
          s << ",";
        }

        s << "{\"name\":\"" << name << "\","
          << "\"description\":\"" << desc << "\","
          << "\"state\":\"" << now_search_engine_index_state_[ name ] << "\"";
        if ( now_search_engine_index_state_[ name ] == "progress" ) {
          s << ",\"steps_done\":" << now_search_engine_index_steps_done_[ name ] << ","
            << "\"steps_count\":" << now_search_engine_index_steps_count_[ name ] << "";
        }
        s << "}";
      }
      s << "]";
      std::string state;
      response.set_status(200);
      response.set_field("Content-Type", "application/json");
      response.set_payload( s.str() );
    } else {
      std::string state;
      response.set_status(400);
      response.set_field("Content-Type", "application/json");
      response.set_payload( "{\"error\":\"indexing not started yet!\"}" );
    }
    return;
  }

  if ( search_engine_command == "engine_exists" ) {
    response.set_status(200);
    response.set_field("Content-Type", "application/json");
    if ( search_engine_exists(search_engine_name, search_engine_version) ) {
      response.set_payload( "{\"yes\":\"search engine exists\"}" );
    } else {
      response.set_payload( "{\"no\":\"search engine does not exist\"}" );
    }
    return;
  }
  */

  // unhandled cases
  response.set_status(400);
}

void vise::search_engine_manager::clear_now_search_engine_index_state(void) {
  now_search_engine_index_steps_done_.clear();
  now_search_engine_index_steps_count_.clear();
  now_search_engine_index_state_.clear();

  for ( std::size_t i = 0; i < search_engine_index_state_name_list_.size(); ++i ) {
    now_search_engine_index_state_[ search_engine_index_state_name_list_.at(i) ] = "not_started";
  }
}

bool vise::search_engine_manager::index_start(const std::string search_engine_name,
                                              const std::string search_engine_version) {
  search_engine_index_thread_running_ = true;
  now_search_engine_name_ = search_engine_name;
  now_search_engine_version_ = search_engine_version;
  clear_now_search_engine_index_state();

  /*
  // @todo: find a way to automatically located these executables
  std::string index_exec   = "/home/tlm/dev/vise/bin/compute_index_v2";
  std::string cluster_exec = "/home/tlm/dev/vise/src/search_engine/relja_retrival/src/v2/indexing/compute_clusters.py";
  boost::filesystem::path config_fn = get_config_filename(search_engine_name, search_engine_version);

  std::ostringstream s;

  // 1. create image filename list
  boost::filesystem::path image_list_fn = get_image_list_filename(search_engine_name, search_engine_version);
  boost::filesystem::path image_data_dir = get_image_data_dir(search_engine_name, search_engine_version);
  s << "cd " << image_data_dir.string() << " && find *.jpg -print -type f > " << image_list_fn.string();

  // @todo: code is not portable for Windows
  std::system( s.str().c_str() );

  bool ok;

  // 2. extract descriptors
  s.str("");
  s.clear();
  s << "mpirun -np 8 " << index_exec
  << " trainDescs " << search_engine_name << " " << config_fn.string();
  now_search_engine_index_state_[ "relja_retrival:trainDescs" ] = "started";
  ok = run_shell_command( "relja_retrival:trainDescs", s.str() );
  if ( ! ok ) {
  search_engine_index_thread_running_ = false;
  return false;
  }

  // 3. cluster descriptors
  s.str("");
  s.clear();
  s << "mpirun -np 8 python " << cluster_exec
  << " " << search_engine_name << " " << config_fn.string() << " 8";
  now_search_engine_index_state_[ "relja_retrival:cluster" ] = "started";
  ok = run_shell_command( "relja_retrival:cluster", s.str() );
  if ( ! ok ) {
  search_engine_index_thread_running_ = false;
  return false;
  }

  // 4. trainAssign
  s.str("");
  s.clear();
  //s << "mpirun -np 8 " << index_exec // see docs/known_issues.txt
  s << index_exec
  << " trainAssign " << search_engine_name << " " << config_fn.string();
  now_search_engine_index_state_[ "relja_retrival:trainAssign" ] = "started";
  ok = run_shell_command( "relja_retrival:trainAssign", s.str() );
  if ( ! ok ) {
  search_engine_index_thread_running_ = false;
  return false;
  }

  // 4. trainHamm
  s.str("");
  s.clear();
  //s << "mpirun -np 8 " << index_exec
  s << index_exec
  << " trainHamm " << search_engine_name << " " << config_fn.string();
  now_search_engine_index_state_[ "relja_retrival:trainHamm" ] = "started";
  ok = run_shell_command( "relja_retrival:trainHamm", s.str() );
  if ( ! ok ) {
  search_engine_index_thread_running_ = false;
  return false;
  }

  // 5. index
  s.str("");
  s.clear();
  s << "mpirun -np 8 " << index_exec
  << " index " << search_engine_name << " " << config_fn.string();
  ok = run_shell_command( "relja_retrival:index", s.str() );
  if ( ! ok ) {
  search_engine_index_thread_running_ = false;
  return false;
  }

  search_engine_index_thread_running_ = false;
  return true;
  }

  bool search_engine_manager::run_shell_command(std::string cmd_name,
  std::string cmd) {
  boost::process::ipstream pipe;
  BOOST_LOG_TRIVIAL(debug) << "running command: {" << cmd_name << "} [" << cmd << "]";
  boost::process::child p(cmd, boost::process::std_out > pipe);
  std::string line;

  while ( pipe && std::getline(pipe, line) && !line.empty() ) {
  BOOST_LOG_TRIVIAL(debug) << "[" << cmd_name << "] " << line;
  if ( vise::util::starts_with(line, "relja_retrival,") ) {
  //trainDescs:relja_retrival,trainDescsManager(images),2018-Jun-25 11:47:52,1,40
  // 0                       , 1                       , 2                  ,3,4
  std::vector<std::string> d = vise::util::split(line, ',');
  now_search_engine_index_steps_done_[ cmd_name ] = d[3];
  now_search_engine_index_steps_count_[cmd_name ] = d[4];
  now_search_engine_index_state_[ cmd_name ] = "progress";
  }
  }
  // @todo check if the process returned non-zero status code
  // @todo process error cases

  p.wait();
  if ( p.exit_code() ) {
  now_search_engine_index_state_[ cmd_name ] = "error";
  return false;
  } else {
  now_search_engine_index_state_[ cmd_name ] = "done";
  return true;
  }
  */
}

std::string vise::search_engine_manager::get_unique_filename(std::string extension) {
  return boost::filesystem::unique_path("%%%%%%%%%%").string() + extension;
}

bool vise::search_engine_manager::search_engine_exists(const std::string search_engine_id) {
  boost::filesystem::path search_engine_data_dir = data_dir_ / search_engine_id;

  if ( boost::filesystem::exists(search_engine_data_dir) ) {
    return true;
  } else {
    return false;
  }
}

bool vise::search_engine_manager::create_search_engine(const std::string search_engine_id) {
}

//
// search engine image i/o
//
bool vise::search_engine_manager::add_image_from_http_payload(const boost::filesystem::path filename,
                                                              const std::string& request_body) {
  try {
    Magick::Blob blob(request_body.c_str(), request_body.size());

    if ( blob.length() ) {
      Magick::Image im(blob);
      im.magick("JPEG");
      im.colorSpace(Magick::sRGBColorspace);
      im.write(filename.string());
      BOOST_LOG_TRIVIAL(debug) << "written file [" << filename.string() << "]";
      return true;
    } else {
      BOOST_LOG_TRIVIAL(debug) << "failed to write file [" << filename.string() << "]";
      return false;
    }
  } catch( std::exception &e ) {
    BOOST_LOG_TRIVIAL(debug) << "exception occured while writing file [" << filename.string() << "] : " << e.what();
    return false;
  }
}

//
// search engine load/unload/maintenance
//
bool vise::search_engine_manager::is_search_engine_loaded(std::string search_engine_id) {
  if ( search_engine_list_.find(search_engine_id) == search_engine_list_.end() ) {
    return false;
  } else {
    return true;
  }
}

void vise::search_engine_manager::load_search_engine(std::string search_engine_id) {
  load_search_engine_mutex_.lock();

  try {
    if ( ! is_search_engine_loaded(search_engine_id) ) {
      boost::filesystem::path se_data_dir  = data_dir_ / search_engine_id;
      boost::filesystem::path se_asset_dir = asset_dir_ / search_engine_id;
      boost::filesystem::path se_temp_dir  = temp_dir_ / search_engine_id;
      vise::search_engine *se = new vise::relja_retrival(search_engine_id,
                                                         se_data_dir,
                                                         se_asset_dir,
                                                         se_temp_dir);
      se->init();
      se->load();
      search_engine_list_.insert( std::pair<std::string, vise::search_engine*>(search_engine_id, se) );
    }
  } catch (std::exception &e) {
    BOOST_LOG_TRIVIAL(debug) << "failed to load search engine [" << search_engine_id << "] : "
                             << e.what();
  }

  load_search_engine_mutex_.unlock();
}

bool vise::search_engine_manager::unload_all_search_engine() {
  // unload all the search engines from search_engine_list_
  std::map<std::string, vise::search_engine* >::iterator it;
  for ( it =  search_engine_list_.begin(); it !=  search_engine_list_.end(); ++it ) {
    BOOST_LOG_TRIVIAL(debug) << "unloading search engine: " << it->second->id();
    it->second->unload();
    delete it->second;
  }
}

//
// search engine query
//

void vise::search_engine_manager::query(const std::string search_engine_id,
                                        const std::string search_engine_command,
                                        const std::map<std::string, std::string> uri_param,
                                        const std::string request_body,
                                        http_response& response) {
  if ( search_engine_command == "_file" ) {
    unsigned int file_id;
    std::string filename = "";
    if ( uri_param.count("file_id") == 1 ) {
      std::stringstream ss;
      ss << uri_param.find("file_id")->second;
      ss >> file_id;
      filename = search_engine_list_[ search_engine_id ]->get_filename(file_id);
    }

    if ( uri_param.count("filename") == 1 ) {
      filename = uri_param.find("filename")->second;
      if ( search_engine_list_[ search_engine_id ]->file_exists(filename) ) {
        file_id = search_engine_list_[ search_engine_id ]->get_file_id(filename);
      } else {
        filename = "";
      }
    }

    if ( filename == "" ) {
      response.set_status(400);
      return;
    }

    // prepare json
    std::ostringstream json;
    json << "{\"search_engine_id\":\"" << search_engine_id << "\","
         << "\"home_uri\":\"" << "/vise/home.html\","
         << "\"image_uri_prefix\":\"" << get_image_uri_prefix(search_engine_id) << "\","
         << "\"image_uri_namespace\":\"image/\","
         << "\"query_uri_prefix\":\"" << get_query_uri_prefix(search_engine_id) << "\","
         << "\"query_search_namespace\":\"_search\","
         << "\"query_file_namespace\":\"_file\","
         << "\"query_filelist_namespace\":\"_filelist\","
         << "\"filename\":\"" << filename << "\","
         << "\"file_id\":" << file_id
         << "}";

    if ( uri_param.count("format") == 0 ) {
      // return html
      std::ostringstream html;
      html << vise::search_engine_manager::RESPONSE_HTML_PAGE_PREFIX
           << "\n<body onload=\"_vise_file()\">"
           << "\n  <script>var _vise_file_str = '" << json.str() << "';\nvar _vise_file = {};</script>"
           << "\n  <script src=\"/vise/_vise_common.js\"></script>"
           << "\n  <script src=\"/vise/_via_min.js\"></script>"
           << "\n  <script src=\"/vise/_vise_file.js\"></script>"
           << "\n  <script>var _VISE_VERSION = '" << VISE_SERVER_VERSION_MAJOR << "."
           << VISE_SERVER_VERSION_MINOR << "." << VISE_SERVER_VERSION_PATCH << "';</script>"
           << "\n</body>\n"
           << vise::search_engine_manager::RESPONSE_HTML_PAGE_SUFFIX;
      response.set_field("Content-Type", "text/html");
      response.set_payload(html.str());
      //BOOST_LOG_TRIVIAL(debug) << "responded with html";
    } else {
      if ( uri_param.find("format")->second == "json" ) {
        response.set_field("Content-Type", "application/json");
        response.set_payload(json.str());
        //BOOST_LOG_TRIVIAL(debug) << "responded with json";
      } else {
        response.set_status(400);
      }
    }

    return;
  }

  if ( search_engine_command == "_filelist" ) {
    std::string filename_regex;
    unsigned int filelist_size;

    std::vector<unsigned int> file_id_list;
    std::vector<std::string> filename_list;

    std::stringstream ss;
    unsigned int from = 0;
    if ( uri_param.count("from") == 1 ) {
      ss << uri_param.find("from")->second;
      ss >> from;
    }

    unsigned int count;
    unsigned int default_count = 1024;
    if ( uri_param.count("count") == 1 ) {
      ss.clear();
      ss.str("");
      ss << uri_param.find("count")->second;
      ss >> count;
    } else {
      count = default_count;
    }

    if ( uri_param.count("filename_regex") == 1 ) {
      unsigned int MAX_RESULT_SIZE = 1000;
      filename_regex = uri_param.find("filename_regex")->second;
      // @todo: find a better way to encode and decode uri
      // filename_regex is url encoded (space -> +), hence decode
      for ( std::size_t i=0; i < filename_regex.length(); ++i ) {
        if ( filename_regex[i] == '+' ) {
          filename_regex[i] = ' ';
        }
      }

      search_engine_list_[ search_engine_id ]->get_filelist(filename_regex, file_id_list);
      filelist_size = file_id_list.size();
    } else {
      search_engine_list_[ search_engine_id ]->get_filelist(file_id_list);
      filelist_size = search_engine_list_[ search_engine_id ]->get_filelist_size();
      filename_regex = "";
    }

    std::string show_from = "0";
    std::string show_count = "45";
    if ( uri_param.count("show_from") == 1 ) {
      show_from = uri_param.find("show_from")->second;
    }
    if ( uri_param.count("show_count") == 1 ) {
      show_count = uri_param.find("show_count")->second;
    }

    // prepare json
    std::ostringstream json;
    json << "{\"search_engine_id\":\"" << search_engine_id << "\","
         << "\"home_uri\":\"" << "/vise/home.html\","
         << "\"image_uri_prefix\":\"" << get_image_uri_prefix(search_engine_id) << "\","
         << "\"image_uri_namespace\":\"image/\","
         << "\"query_uri_prefix\":\"" << get_query_uri_prefix(search_engine_id) << "\","
         << "\"FILELIST_SIZE\":" << filelist_size << ","
         << "\"from\":" << from << ","
         << "\"count\":" << count << ","
         << "\"show_from\":" << show_from << ","
         << "\"show_count\":" << show_count << ","
         << "\"filename_regex\":\"" << filename_regex << "\","
         << "\"file_id_list_subset\":[";
    uint32_t file_id;
    std::string filename_uri;
    for ( uint32_t i = from; i < (from + count) && (i < filelist_size); ++i ) {
      if ( i != from ) {
        json << ",";
      }
      json << file_id_list[i];
      /*
      json << "{\"file_id\":" << file_id_list[i] << ","
           << "\"filename\":\"" << filename_list[i] << "\"}";
      */
    }
    json << "]}";

    if ( uri_param.count("format") == 0 ) {
      // return html
      std::ostringstream html;
      html << vise::search_engine_manager::RESPONSE_HTML_PAGE_PREFIX
           << "\n<body onload=\"_vise_filelist()\">"
           << "\n  <script>var _vise_filelist_str = '" << json.str() << "';\nvar _vise_filelist = {};</script>"
           << "\n  <script src=\"/vise/_vise_common.js\"></script>"
           << "\n  <script src=\"/vise/_vise_filelist.js\"></script>"
           << "\n  <script>var _VISE_VERSION = '" << VISE_SERVER_VERSION_MAJOR << "."
           << VISE_SERVER_VERSION_MINOR << "." << VISE_SERVER_VERSION_PATCH << "';</script>"
           << "\n</body>\n"
           << vise::search_engine_manager::RESPONSE_HTML_PAGE_SUFFIX;
      response.set_field("Content-Type", "text/html");
      response.set_payload(html.str());
      //BOOST_LOG_TRIVIAL(debug) << "responded with html containing " << file_id_list.size() << " entries";
    } else {
      if ( uri_param.find("format")->second == "json" ) {
        response.set_field("Content-Type", "application/json");
        response.set_payload(json.str());
        //BOOST_LOG_TRIVIAL(debug) << "responded with json containing " << file_id_list.size() << " entries";
      } else {
        response.set_status(400);
      }
    }
    return;
  }

  if ( search_engine_command == "_search" ) {
    // validation
    if ( uri_param.count("file_id") != 1 ) {
      response.set_status(400);
      return;
    }
    if ( uri_param.count("x")      != 1 ||
         uri_param.count("y")      != 1 ||
         uri_param.count("width")  != 1 ||
         uri_param.count("height") != 1 ) {
      response.set_status(400);
      return;
    }

    unsigned int file_id;
    std::stringstream ss;
    ss << uri_param.find("file_id")->second;
    ss >> file_id;

    unsigned int x, y, width, height;
    ss.clear();
    ss.str("");
    ss << uri_param.find("x")->second;
    ss >> x;
    ss.clear();
    ss.str("");
    ss << uri_param.find("y")->second;
    ss >> y;
    ss.clear();
    ss.str("");
    ss << uri_param.find("width")->second;
    ss >> width;
    ss.clear();
    ss.str("");
    ss << uri_param.find("height")->second;
    ss >> height;

    ss.clear();
    ss.str("");
    unsigned int from = 0;
    if ( uri_param.count("from") == 1 ) {
      ss << uri_param.find("from")->second;
      ss >> from;
    }

    ss.clear();
    ss.str("");
    unsigned int count = 1024;
    if ( uri_param.count("count") == 1 ) {
      ss << uri_param.find("count")->second;
      ss >> count;
    }

    ss.clear();
    ss.str("");
    double score_threshold = 0;
    if ( uri_param.count("score_threshold") == 1 ) {
      ss << uri_param.find("score_threshold")->second;
      ss >> score_threshold;
    }

    std::string show_from = "0";
    std::string show_count = "45";
    if ( uri_param.count("show_from") == 1 ) {
      show_from = uri_param.find("show_from")->second;
    }
    if ( uri_param.count("show_count") == 1 ) {
      show_count = uri_param.find("show_count")->second;
    }

    std::vector<unsigned int> result_file_id;
    std::vector<float> result_score;
    std::vector< std::array<double, 9> > result_H;
    std::vector<std::string> result_filename;
    std::vector<std::string> result_metadata;

    search_engine_list_[ search_engine_id ]->query_using_file_region(file_id,
                                                                     x, y, width, height, score_threshold,
                                                                     result_file_id, result_filename,
                                                                     result_metadata, result_score, result_H);
    std::ostringstream json;
    std::string query_filename = search_engine_list_[ search_engine_id ]->get_filename(file_id);
    json << "{\"search_engine_id\":\"" << search_engine_id << "\","
         << "\"query\":{\"file_id\":" << file_id << ","
         << "\"score_threshold\":" << score_threshold << ","
         << "\"filename\":\"" << query_filename << "\","
         << "\"x\":" << x << ",\"y\":" << y << ",\"width\":" << width << ",\"height\":" << height << ","
         << "\"from\":" << from << ",\"count\":" << count << ","
         << "\"show_from\":" << show_from << ",\"show_count\":" << show_count << "},"
         << "\"home_uri\":\"" << "/vise/home.html\","
         << "\"image_uri_prefix\":\"" << get_image_uri_prefix(search_engine_id) << "\","
         << "\"image_uri_namespace\":\"image/\","
         << "\"query_uri_prefix\":\"" << get_query_uri_prefix(search_engine_id) << "\","
         << "\"QUERY_RESULT_SIZE\":" << result_file_id.size() << ","
         << "\"query_result_subset\":[";

    if ( count == 0 ) {
      // indicates to return all results
      from = 0;
      count = result_score.size();
    }

    for ( std::size_t i = from; (i < result_score.size()) && (i < (from + count)); ++i ) {
      if ( i != from ) {
        json << ",";
      }
      json << "{\"file_id\":" << result_file_id[i] << ","
           << "\"filename\":\"" << result_filename[i] << "\","
           << "\"metadata\":\"" << result_metadata[i] << "\","
           << "\"score\":" << result_score[i] << ","
           << "\"H\":[" << result_H[i][0] << "," << result_H[i][1] << "," << result_H[i][2] << ","
           << result_H[i][3] << "," << result_H[i][4] << "," << result_H[i][5] << ","
           << result_H[i][6] << "," << result_H[i][7] << "," << result_H[i][8] << "]}";
    }
    json << "]}";

    if ( uri_param.count("format") == 0 ) {
      // return html
      std::ostringstream html;
      html << vise::search_engine_manager::RESPONSE_HTML_PAGE_PREFIX
           << "\n<body onload=\"_vise_search()\">"
           << "\n  <script>var _vise_search_result_str = '" << json.str() << "';\nvar _vise_search_result = {};</script>"
           << "\n  <script src=\"/vise/_vise_common.js\"></script>"
           << "\n  <script src=\"/vise/_vise_search.js\"></script>"
           << "\n  <script>var _VISE_VERSION = '" << VISE_SERVER_VERSION_MAJOR << "."
           << VISE_SERVER_VERSION_MINOR << "." << VISE_SERVER_VERSION_PATCH << "';</script>"
           << "\n</body>\n"
           << vise::search_engine_manager::RESPONSE_HTML_PAGE_SUFFIX;
      response.set_field("Content-Type", "text/html");
      response.set_payload(html.str());
      //BOOST_LOG_TRIVIAL(debug) << "responded with html containing " << result_score.size() << " entries";
    } else {
      if ( uri_param.find("format")->second == "json" ) {
        response.set_field("Content-Type", "application/json");
        response.set_payload(json.str());
        //BOOST_LOG_TRIVIAL(debug) << "responded with json containing " << result_score.size() << " entries";
      } else {
        response.set_status(400);
      }
    }
    return;
  }

  if ( search_engine_command == "_register" ) {
    // validation
    if ( uri_param.count("file1_id") != 1 ||
         uri_param.count("x1") != 1 ||
         uri_param.count("y1") != 1 ||
         uri_param.count("width1") != 1 ||
         uri_param.count("height1") != 1 ||
         uri_param.count("file2_id") != 1 ) {
      std::cout << "\nmissing arguments" << std::flush;
      response.set_status(400);
      return;
    }

    unsigned int file1_id, file2_id;
    unsigned int x1, y1, width1, height1;

    std::stringstream ss;
    ss << uri_param.find("file1_id")->second;
    ss >> file1_id;

    ss.clear();
    ss.str("");
    ss << uri_param.find("file2_id")->second;
    ss >> file2_id;

    ss.clear();
    ss.str("");
    ss << uri_param.find("x1")->second;
    ss >> x1;

    ss.clear();
    ss.str("");
    ss << uri_param.find("y1")->second;
    ss >> y1;

    ss.clear();
    ss.str("");
    ss << uri_param.find("width1")->second;
    ss >> width1;

    ss.clear();
    ss.str("");
    ss << uri_param.find("height1")->second;
    ss >> height1;

    // perform image registration
    std::string im1_fn = search_engine_list_[ search_engine_id ]->get_filename_absolute_path(file1_id);
    std::string im2_fn = search_engine_list_[ search_engine_id ]->get_filename_absolute_path(file2_id);
    Eigen::MatrixXd Hopt;
    std::size_t fp_match_count;
    bool success = false;
    Magick::Image im2_crop_tx( Magick::Geometry(width1, height1, 0, 0), "white");

    try {
      // @todo: use H_initial as the initial guess for homography
      imreg_sift::ransac_dlt(im1_fn.c_str(), im2_fn.c_str(),
                             x1, y1, width1, height1,
                             Hopt, fp_match_count, im2_crop_tx, success);
    } catch( std::exception& e ) {
      //std::cout << "\nexception: " << e.what() << std::flush;;
      response.set_status(400);
      return;
    }

    if ( ! success ) {
      //std::cout << "\nsuccess=" << success << std::flush;;
      response.set_status(400);
      return;
    }

    // extract image data
    Magick::Blob blob;
    im2_crop_tx.magick("JPEG");
    im2_crop_tx.colorSpace(Magick::sRGBColorspace);
    im2_crop_tx.write( &blob );

    std::string img_data( (char *) blob.data(), blob.length() );
    response.set_field("Content-Type", "image/jpeg");
    response.set_payload( img_data );
    BOOST_LOG_TRIVIAL(debug) << "responded with registered jpeg image of size " << blob.length();
    return;
  }
}


//
// search engine admin
//
void vise::search_engine_manager::admin(const std::string command,
                                        const std::map<std::string, std::string> uri_param,
                                        const std::string request_body,
                                        http_response& response) {
  BOOST_LOG_TRIVIAL(debug) << "admin command: [" << command << "]";
}

//
// asset i/o
//

void vise::search_engine_manager::asset(const std::string search_engine_id,
                                        const std::string asset_type,
                                        const std::string asset_name,
                                        const std::map<std::string, std::string> uri_param,
                                        const std::string request_body,
                                        http_response& response) {
  std::string file_abs_path;
  if ( asset_name.find(".") == std::string::npos ) {
    // asset_name contains file_id
    unsigned int file_id;
    std::stringstream ss(asset_name);
    ss >> file_id;

    file_abs_path = search_engine_list_[ search_engine_id ]->get_filename_absolute_path(file_id);

    std::string header_filename = "inline; filename=\"";
    header_filename += search_engine_list_[ search_engine_id ]->get_filename(file_id);
    header_filename += "\"";
    response.set_field("Content-Disposition", header_filename);
  } else {
    file_abs_path = search_engine_list_[ search_engine_id ]->get_filename_absolute_path(asset_name);
  }

  //BOOST_LOG_TRIVIAL(debug) << "serving file_abs_path [" << file_abs_path << "]";
  std::string file_content;
  bool ok = vise::util::load_file_content(boost::filesystem::path(file_abs_path), file_content);
  if ( ok ) {
    response.set_payload( file_content );
    response.set_content_type_from_filename( file_abs_path );
    response.set_field("Access-Control-Allow-Origin", "*"); // enable CORS for images
    //BOOST_LOG_TRIVIAL(debug) << "http response contains file [" << file_abs_path << "]";
  } else {
    response.set_status(400);
    BOOST_LOG_TRIVIAL(debug) << "failed to send file in http response [" << file_abs_path << "]";
  }
}

//
// util
//
std::string vise::search_engine_manager::get_image_uri_prefix(std::string search_engine_id) {
  std::string prefix = "/vise/asset/";
  return prefix + search_engine_id + "/";
}

std::string vise::search_engine_manager::get_query_uri_prefix(std::string search_engine_id) {
  std::string prefix = "/vise/query/";
  return prefix + search_engine_id + "/";
}
