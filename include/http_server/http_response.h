/** @file   http_response.h
 *  @brief  Data structure to store http response
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   17 Nov 2017
 */

#ifndef _HTTP_RESPONSE_H_
#define _HTTP_RESPONSE_H_

#include <iostream>
#include <sstream>
#include <map>

using namespace std;

class http_response {
  public:
  string status_;
  map<string,string> fields_;
  string payload_;

  http_response() {
    status_ = "HTTP/1.1 200 OK";
  }

  void set_status(unsigned int response_code) {
    status_ = "HTTP/1.1 ";
    switch(response_code) {
    case 100:
      status_ += "100 Continue";
      break;
    case 200:
      status_ += "200 OK";
      break;
    case 303:
      status_ += "303 See Other";
      break;
    case 404:
      status_ += "404 Not Found";
      break;
    case 400:
      status_ += "400 Bad Request";
      break;
    default:
      status_ += "400 Bad Request";
    }
  }
  void set_field(string name, string value) {
    fields_.insert( make_pair(name, value) );
  }

  void set_content_type_from_filename(string filename) {
    string ctype = "application/octet-stream";
    size_t dot = filename.rfind(".");
    string ext = filename.substr(dot + 1);
    if( ext == "html" ) {
      ctype = "text/html";
    } 
    else if( ext == "jpg" ) {
      ctype = "image/jpeg";
    }
    else if( ext == "png" ) {
      ctype = "image/png";
    }
    else if( ext == "css" ) {
      ctype = "text/css";
    }
    else if( ext == "js" ) {
      ctype = "application/javascript";
    }
    else if( ext == "txt" ) {
      ctype = "text/plain";
    }
    else if( ext == "ico" ) {
      ctype = "image/x-icon";
    }
    else if( ext == "json" ) {
      ctype = "application/json";
    }

    set_field("Content-Type", ctype);
  }

  void set_payload(string payload) {
    payload_ = payload;
    fields_["Content-Length"] = to_string(payload_.length());
  }

  string get_response_str() {
    ostringstream s;
    s << status_ << "\r\n";
    for( auto it = fields_.begin(); it != fields_.end(); it++ ) {
      s << it->first << ": " << it->second << "\r\n";
    }
    s << "\r\n" << payload_;
    return s.str();
  }
};

#endif
