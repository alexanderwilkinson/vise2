/** @file   vise_request_handler.h
 *  @brief  A singleton class which handles http requests related to vise
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   20 June 2018
 */

#include "vise/util.h"

//
// string util
//
bool vise::util::starts_with(const std::string &s, const std::string prefix) {
  if ( s.substr(0, prefix.length()) == prefix ) {
    return true;
  } else {
    return false;
  }
}

bool vise::util::ends_with(const std::string &s, const std::string suffix) {
  size_t pos = s.length() - suffix.length();
  if ( s.substr( pos ) == suffix ) {
    return true;
  } else {
    return false;
  }
}

bool vise::util::contains(const std::string &s, const std::string substr) {
  if ( s.find(substr) == std::string::npos ) {
    return false;
  } else {
    return true;
  }
}

std::vector<std::string> vise::util::split(const std::string &s, const char separator) {
  std::vector<std::string> chunks;
  std::vector<std::size_t> seperator_index_list;

  std::size_t start = 0;
  std::size_t sep_index;
  while ( start < s.length() ) {
    sep_index = s.find(separator, start);
    if ( sep_index == std::string::npos ) {
      break;
    } else {
      chunks.push_back( s.substr(start, sep_index - start) );
      start = sep_index + 1;
    }
  }
  if ( start != s.length() ) {
    chunks.push_back( s.substr(start) );
  }
  return chunks;
}

std::vector<std::string> vise::util::split(const std::string &s, const char separator, const std::string stop_string) {
  std::vector<std::string> chunks;
  std::vector<std::size_t> seperator_index_list;

  std::size_t start = 0;
  std::size_t sep_index;
  std::size_t stop_index = s.find(stop_string);
  if ( stop_index == std::string::npos ) {
    stop_index = s.length();
  }

  while ( start < stop_index ) {
    sep_index = s.find(separator, start);
    if ( sep_index == std::string::npos ) {
      break;
    } else {
      if ( sep_index < stop_index ) {
        chunks.push_back( s.substr(start, sep_index - start) );
        start = sep_index + 1;
      } else {
        break;
      }
    }
  }
  if ( start != stop_index ) {
    chunks.push_back( s.substr(start) );
  }
  return chunks;
}

void vise::util::decompose_uri(const std::string &uri, std::vector<std::string>& uri_components, std::map<std::string, std::string>& uri_param) {
  uri_components = vise::util::split(uri, '/', "?");
  std::size_t n = uri_components.size();

  std::size_t param_start_index = uri_components.at(n-1).find('?');
  if ( param_start_index != std::string::npos ) {
    std::string param_str = uri_components.at(n-1).substr( param_start_index + 1 );
    uri_components.at(n-1) = uri_components.at(n-1).substr(0, param_start_index);
    std::vector<std::string> param_list = vise::util::split(param_str, '&');

    std::size_t np = param_list.size();
    if ( np ) {
      uri_param.clear();
      for ( std::size_t i = 0; i < np; ++i ) {
        std::vector<std::string> pi = vise::util::split(param_list.at(i), '=');
        if ( pi.size() == 2 ) {
          uri_param[ pi[0] ] = pi[1];
        }
      }
    }
  }
}

//
// file i/o
//
bool vise::util::load_file_content(const boost::filesystem::path fn, std::string& file_content) {
  if( !boost::filesystem::exists(fn) ) {
    BOOST_LOG_TRIVIAL(debug) << "file does not exist [" << fn.string() << "]";
    return false;
  }
  if( !boost::filesystem::is_regular_file(fn) ) {
    BOOST_LOG_TRIVIAL(debug) << "not a regular file [" << fn.string() << "]";
    return false;
  }

  try {
    std::ifstream f;
    f.open(fn.string().c_str(), std::ios::in | std::ios::binary);
    f.seekg(0, std::ios::end);
    file_content.clear();
    file_content.reserve( f.tellg() );
    f.seekg(0, std::ios::beg);
    file_content.assign( (std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>() );
    f.close();
    return true;
  } catch(std::exception &e) {
    BOOST_LOG_TRIVIAL(error) << "failed to load file [" << fn.string() << "]";
    return false;
  }
}

//
// http uri parsing util
//


bool vise::util::has_special_char(const std::string &s) {
  if ( s.find('!') != std::string::npos )
    return true;
  if ( s.find('"') != std::string::npos )
    return true;
  if ( s.find('$') != std::string::npos )
    return true;
  if ( s.find('%') != std::string::npos )
    return true;
  if ( s.find('^') != std::string::npos )
    return true;
  if ( s.find('&') != std::string::npos )
    return true;
  if ( s.find('*') != std::string::npos )
    return true;
  if ( s.find('!') != std::string::npos )
    return true;
  if ( s.find('@') != std::string::npos )
    return true;
  if ( s.find('~') != std::string::npos )
    return true;
  if ( s.find('?') != std::string::npos )
    return true;
  if ( s.find("../") != std::string::npos )
    return true;
  if ( s.find("./") != std::string::npos )
    return true;
  if ( s.find(".\\") != std::string::npos )
    return true;

  /*  if ( s.find('/') != std::string::npos )
    return true;
  if ( s.find('\\') != std::string::npos )
    return true;
  */

  return false;
}

//
// i/o
//
void vise::util::print_vector( std::string name, std::vector<std::string>& v ) {
  std::ostringstream s;
  for ( std::size_t i = 0; i < v.size(); ++i ) {
    s << v[i] << ",";
  }
  BOOST_LOG_TRIVIAL(debug) << name << " = [" << s.str() << "]";
}

void vise::util::print_map( std::string name, std::map<std::string, std::string>& m ) {
  std::ostringstream s;
  std::map<std::string, std::string>::iterator it;

  for ( it = m.begin(); it != m.end(); ++it ) {
    s << it->first << "=" << it->second << ", ";
  }
  BOOST_LOG_TRIVIAL(debug) << name << " = [" << s.str() << "]";
}
