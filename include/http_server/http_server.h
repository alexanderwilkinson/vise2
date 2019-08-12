/** @file   http_server.h
 *  @brief  A generic http server inspired from the Boost asio HTTP Server
 *          http://www.boost.org/doc/libs/1_65_0/doc/html/boost_asio/examples/cpp11_examples.html
 *
 *
 *  @author Abhishek Dutta (adutta@robots.ox.ac.uk)
 *  @date   14 Nov. 2017
 */

#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <signal.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio/signal_set.hpp>

#include "http_server/connection.h"
#include "http_server/http_request_handler.h"

class http_server : private boost::noncopyable {
 public:
  http_server(const std::string& address,
              const std::string& port,
              const std::size_t thread_pool_size);
  void start();
  void stop();
 private:
  void accept_new_connection();
  void handle_connection( const boost::system::error_code& e );

  std::size_t thread_pool_size_;
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::shared_ptr<connection> new_connection_;

  boost::asio::signal_set signals_;
};
#endif
