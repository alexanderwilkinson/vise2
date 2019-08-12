/** @file   http_request.h
 *  @brief  Data structure and algorithm to parse and store http request
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   15 Nov 2017
 */

#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_

#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

enum class parser_state {WAITING_FOR_FIRST_REQUEST_CHUNK,
    WAITING_FOR_HTTP_METHOD,
    WAITING_FOR_REQUEST_URI,
    WAITING_FOR_HTTP_VERSION,
    VERSION_SEEN_WAITING_FOR_CR,
    VERSION_SEEN_WAITING_FOR_LF,
    WAITING_FOR_HTTP_HEADER_FIELD,
    WAITING_FOR_HTTP_FIELD_VALUE,
    WAITING_FOR_END_OF_HEADER_LF,
    HEADER_SEEN_WAITING_FOR_PAYLOAD,
    REQUEST_COMPLETE
};


class http_request {
  private:
  parser_state parser_state_;
  bool header_has_expect_100_continue;
  std::size_t content_length_;
  string unset_field_name_;
  size_t payload_size_;

  public:
  http_request() {
    parser_state_ = parser_state::WAITING_FOR_FIRST_REQUEST_CHUNK;
    header_has_expect_100_continue = false;
  };

  string method_;
  string uri_;
  string version_;

  map<string, string> fields_;
  map<string, string> multipart_data_;
  map<string, string> uri_arg_;

  ostringstream payload_;


  void reset_expect_100_continue_header() {
    header_has_expect_100_continue = false;
  }
  bool get_expect_100_continue_header() const {
    return header_has_expect_100_continue;
  }

  size_t payload_size() const {
    return payload_size_;
  }

  void parse(string request_chunk);
  string print(bool include_payload=true) const;
  string current_state_name() const;
  void set_state_request_complete();
  bool is_request_complete() const;
  bool is_header_field_present(string field_name) const;
  string header_field_value(string field_name) const;

  bool parse_multipart_form_data();
  size_t multipart_data_size() const {
    return multipart_data_.size();
  }
  bool exists_multipart_data_name(string name) const;
  string multipart_data_value(string name) const;

  bool parse_uri(map<string,string>& uri_arg) const;
};

#endif
