#include "http_server/http_request.h"

string http_request::print(bool include_payload) const {
  ostringstream s;
  s << "{" << method_ << " [" << uri_ << "]" << " " << version_ << "; {";
  for ( auto it = fields_.begin(); it != fields_.end(); it++ ) {
    s << "[" << it->first << ":" << it->second << "]";
  }
  s << "}";
  s << "{payload=" << payload_size() << " bytes}";
  s << "{parser_state_=" << current_state_name() << "}";
  /*
  if(include_payload) {
    s << "{payload=" << payload_.str() << "}";
  } else {
    s << "{payload=" << payload_size() << " bytes}";
  }
  s << "}";
  */
  return s.str();
}

void http_request::parse(string request_chunk) {
  if ( parser_state_ == parser_state::WAITING_FOR_FIRST_REQUEST_CHUNK ) {
    if ( request_chunk.length() ) {
      parser_state_ = parser_state::WAITING_FOR_HTTP_METHOD;
    }
  }

  if ( parser_state_ == parser_state::HEADER_SEEN_WAITING_FOR_PAYLOAD ) {
    payload_ << request_chunk;
    if ( ((size_t) payload_.tellp()) == content_length_ ) {
      set_state_request_complete();
    }
    return;
  }

  std::size_t extract = 0;
  for ( std::size_t i = 0; i != request_chunk.length(); i++ ) {
    //cout << "\nhttp_request::parse() : Processing character : " << request_chunk[i] << ", before state=" << current_state_name() << flush;
    switch( request_chunk[i] ) {
    case ' ':
      if ( parser_state_ == parser_state::WAITING_FOR_HTTP_METHOD ) {
        method_ = request_chunk.substr(extract, i - extract);
        extract = i + 1;
        parser_state_ = parser_state::WAITING_FOR_REQUEST_URI;
        continue;
      }
      if ( parser_state_ == parser_state::WAITING_FOR_REQUEST_URI ) {
        uri_ = request_chunk.substr(extract, i - extract);
        extract = i + 1;
        parser_state_ = parser_state::WAITING_FOR_HTTP_VERSION;
        continue;
      }
      break;

    case '\r':
      if ( parser_state_ == parser_state::WAITING_FOR_HTTP_VERSION ) {
        version_ = request_chunk.substr(extract, i - extract);
        parser_state_ = parser_state::VERSION_SEEN_WAITING_FOR_LF;
        continue;
      }
      if ( parser_state_ == parser_state::WAITING_FOR_HTTP_FIELD_VALUE ) {
        std::string field_value = request_chunk.substr(extract, i - extract);
        if ( field_value[0] == ' ' ) {
          field_value = field_value.substr(1); // remove prefix space
        }
        auto it = fields_.find(unset_field_name_);
        if( it != fields_.end() ) {
          it->second = field_value;
          unset_field_name_ = "";
        } else {
          std::cerr << "\nmalformed http header value: " << field_value << flush;
        }
        extract = i + 1;
        // to prepare for next (if any) http header field
        parser_state_ = parser_state::VERSION_SEEN_WAITING_FOR_LF;
        continue;
      }
      if ( parser_state_ == parser_state::WAITING_FOR_HTTP_HEADER_FIELD ) {
        parser_state_ = parser_state::WAITING_FOR_END_OF_HEADER_LF;
        continue;
      }

      break;
    case '\n':
      if ( parser_state_ == parser_state::VERSION_SEEN_WAITING_FOR_LF ) {
        extract = i + 1;
        parser_state_ = parser_state::WAITING_FOR_HTTP_HEADER_FIELD;
        continue;
      }
      if ( parser_state_ == parser_state::WAITING_FOR_END_OF_HEADER_LF ) {
        //cout << "\nfull header seen" << flush;

        // browsers expect 100-continue response before sending large files
        if ( is_header_field_present("Expect") ) {
          if ( header_field_value("Expect") == "100-continue" ) {
            header_has_expect_100_continue = true;
          }
        }

        if ( method_ == "POST" && is_header_field_present("Content-Length") ) {
          std::istringstream s(header_field_value("Content-Length"));
          s >> content_length_;
          payload_ << request_chunk.substr(i+1);

          if ( ((size_t) payload_.tellp()) == content_length_ ) {
            set_state_request_complete();
            return;
          } else {
            parser_state_ = parser_state::HEADER_SEEN_WAITING_FOR_PAYLOAD;
            continue;
          }          
        } else {
          parser_state_ = parser_state::REQUEST_COMPLETE;
        }
        continue;
      }

      break;
    case ':':
      if ( parser_state_ == parser_state::WAITING_FOR_HTTP_HEADER_FIELD ) {
        unset_field_name_ = request_chunk.substr(extract, i - extract);
        fields_.insert( make_pair(unset_field_name_, "_value_not_set_") );
        extract = i + 1;
        parser_state_ = parser_state::WAITING_FOR_HTTP_FIELD_VALUE;
        continue;
      }
      break;
    default:
      continue;
    }
    //cout << ", after state=" << current_state_name() << flush;
  }
}

void http_request::set_state_request_complete() {
  payload_size_ = payload_.tellp();
  parser_state_ = parser_state::REQUEST_COMPLETE;
}

bool http_request::is_request_complete() const {
  if ( parser_state_ == parser_state::REQUEST_COMPLETE ) {
    return true;
  } else {
    return false;
  }
}

bool http_request::is_header_field_present(string field_name) const {
  const map<string, string>::const_iterator it = fields_.find(field_name);
  if ( it != fields_.end() ) {
    return true;
  } else {
    return false;
  }
}

string http_request::header_field_value(string field_name) const {
  map<string, string>::const_iterator it = fields_.find(field_name);
  if ( it != fields_.end() ) {
    return it->second;
  } else {
    return "";
  }
}

string http_request::current_state_name() const {
  switch(parser_state_) {
  case parser_state::WAITING_FOR_FIRST_REQUEST_CHUNK:
    return "WAITING_FOR_FIRST_REQUEST_CHUNK";
    break;
  case parser_state::WAITING_FOR_HTTP_METHOD:
    return "WAITING_FOR_HTTP_METHOD";
    break;
  case parser_state::WAITING_FOR_REQUEST_URI:
    return "WAITING_FOR_REQUEST_URI";
    break;
  case parser_state::WAITING_FOR_HTTP_VERSION:
    return "WAITING_FOR_HTTP_VERSION";
    break;
  case parser_state::VERSION_SEEN_WAITING_FOR_CR:
    return "VERSION_SEEN_WAITING_FOR_CR";
    break;
  case parser_state::VERSION_SEEN_WAITING_FOR_LF:
    return "VERSION_SEEN_WAITING_FOR_LF";
    break;
  case parser_state::WAITING_FOR_HTTP_HEADER_FIELD:
    return "WAITING_FOR_HTTP_HEADER_FIELD";
    break;
  case parser_state::WAITING_FOR_HTTP_FIELD_VALUE:
    return "WAITING_FOR_HTTP_FIELD_VALUE";
    break;
  case parser_state::WAITING_FOR_END_OF_HEADER_LF:
    return "WAITING_FOR_END_OF_HEADER_LF";
    break;
  case parser_state::HEADER_SEEN_WAITING_FOR_PAYLOAD:
    return "HEADER_SEEN_WAITING_FOR_PAYLOAD";
    break;
  case parser_state::REQUEST_COMPLETE:
    return "REQUEST_COMPLETE";
    break;
  default:
    return "UNKNOWN STATE";
  }
}

///
/// Multipart Form Data
///

// see : https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Disposition
bool http_request::parse_multipart_form_data() {
  string ctype = header_field_value("Content-Type");
  size_t bspos = ctype.find("boundary=");
  if( bspos == std::string::npos ) {
    return false;
  }
  //cout << "\nbpos = " << bspos << flush;

  string bs = ctype.substr(bspos + string("boundary=").length());
  size_t bslen = bs.length(); // length of boundary string
  //cout << "\nbs=" << bs << ", bslen=" << bslen << flush;

  string payload_str = payload_.str();

  vector<size_t> bloc; // locations of boundary strings
  size_t bp = payload_str.find(bs);
  size_t old_bp = 0;
  do {
    bp = payload_str.find(bs, old_bp);
    if ( bp != std::string::npos ) {
      //cout << "\nbloc i =" << bp << flush;
      bloc.push_back(bp);
      old_bp = bp + bslen;
    }
  } while( bp != std::string::npos );
  //cout << "\nbloc.size() = " << bloc.size() << flush;

  if ( bloc.size() == 0 ) {
    return false;
  }

  size_t crlf_len = 2; // length of \r\n
  for(size_t i=0; i < bloc.size() - 1; i++) {
    //cout << "\nProcessing multipart-data block " << i << flush;

    size_t block_begin  = bloc.at(i) + bslen + crlf_len; // discard the trailing \r\n in first (n-1) boundaries
    size_t block_end    = bloc.at(i+1) - crlf_len;       // discard the preceding \r\n for n boundaries
    size_t block_length = block_end - block_begin;
    //cout << "\nblock_begin=" << block_begin << ", block_end=" << block_end << ", block_length=" << block_length << flush;

    string block = payload_str.substr(block_begin, block_length);
    //cout << "\nblock = {" << block << "}" << flush;

    size_t name1 = block.find("name=");
    size_t name2 = block.find("\"", name1);
    size_t name3 = block.find("\"", name2 + 1);
    if ( name1 == std::string::npos || name2 == std::string::npos || name3 == std::string::npos ) {
      return false;
    }
    //cout << "\nname1=" << name1 << ", name2=" << name2 << ", name3=" << name3 << flush;

    string block_name = block.substr(name2 + 1, name3 - name2 - 1);
    //cout << "\nblock_name=" << block_name << flush;

    size_t value1 = block.find("\r\n\r\n");
    size_t value2 = block.rfind("\r\n");
    if ( value1 == std::string::npos || value2 == std::string::npos ) {
      return false;
    }
    //cout << "\nvalue1=" << value1 << ", value2=" << value2 << flush;
    string block_value = block.substr(value1 + 4, value2 - value1 - 4); // discard the trailing \r\n

    multipart_data_.insert( make_pair(block_name, block_value) );
/*
    if( block_value.length() > 16 ) {
      cout << "\n" << block_name << " : {" << block_value.length() << " bytes}" << flush;
    } else {
      cout << "\n" << block_name << " : {" << block_value << "}" << flush;
    }
*/
  }
  return true;
}

bool http_request::exists_multipart_data_name(string name) const {
  const map<string, string>::const_iterator it = multipart_data_.find(name);
  if ( it != multipart_data_.end() ) {
    return true;
  } else {
    return false;
  }
}

string http_request::multipart_data_value(string name) const {
  map<string, string>::const_iterator it = multipart_data_.find(name);
  if ( it != multipart_data_.end() ) {
    return it->second;
  } else {
    return "";
  }
}

// /imcomp/_compare?file1=id1&file2=id2&region1=x0,y0,x1,y1
bool http_request::parse_uri(map<string,string>& uri_arg) const {
  uri_arg.clear();
  size_t start = uri_.find("?");
  //cout << "\nparsing uri " << uri_ << ", start=" << start << flush;

  if( start != string::npos ) {
    bool parsing_key = true;
    start = start + 1;
    string key;
    string value;
    for( size_t i=start; i<uri_.length(); i++ ) {
      if( uri_.at(i) == '=' && parsing_key) {
        key = uri_.substr(start, i - start);
        start = i + 1;
        parsing_key = false;
      }
      if( uri_.at(i) == '&' && !parsing_key) {
        value = uri_.substr(start, i - start);
        uri_arg.insert( pair<string,string>(key, value) );
        //cout << "\n  " << key << ":" << value << flush;
        start = i + 1;
        parsing_key = true;
      }
    }
    value = uri_.substr(start);
    uri_arg.insert( pair<string,string>(key, value) );
    return true;
  } else {
    return false;
  }
}

