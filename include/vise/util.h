/** @file   vise_request_handler.h
 *  @brief  A singleton class which handles http requests related to vise
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   20 June 2018
 */

#ifndef _VISE_UTIL_H_
#define _VISE_UTIL_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

#define BOOST_LOG_DYN_LINK 1
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

namespace vise {
  namespace util {
    //
    // string util
    //
    bool starts_with(const std::string &s, const std::string prefix);
    bool ends_with(const std::string &s, const std::string suffix);
    bool contains(const std::string &s, const std::string substr);
    std::vector<std::string> split(const std::string &s, const char separator);
    std::vector<std::string> split(const std::string &s, const char separator, const std::string stop_string);
    void decompose_uri(const std::string &uri, std::vector<std::string>& uri_components, std::map<std::string, std::string>& uri_param);

    //
    // file i/o
    //
    bool load_file_content(const boost::filesystem::path fn, std::string& file_content);

    //
    // http uri parsing util
    //
    bool has_special_char(const std::string &s);

    //
    // i/o
    //
    void print_vector( std::string name, std::vector<std::string>& v );
    void print_map( std::string name, std::map<std::string, std::string>& m );
  } // end of namespace util
} // end of namespace vise

#endif
