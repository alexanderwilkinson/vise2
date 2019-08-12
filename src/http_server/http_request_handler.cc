#include "http_server/http_request_handler.h"

http_request_handler *http_request_handler::http_request_handler_ = NULL;

http_request_handler* http_request_handler::instance() {
  if ( !http_request_handler_ ) {
    http_request_handler_ = new http_request_handler;
  }
  return http_request_handler_;
}

void http_request_handler::init() {
  cout << "\nhttp_request_handler::init() - initializing http request handler" << flush;
}

void http_request_handler::handle_http_request(const http_request& request, http_response& response) {
  cout << "\ndummy_request_handler:: handling the following request:\n";
  cout << request.print() << flush;
  response.set_status(200);
  cout << "\nrequest handling completed" << flush;
}
