#include "http_server/http_server.h"

http_server::http_server(const std::string& address,
                         const std::string& port,
                         const std::size_t thread_pool_size) :  thread_pool_size_( thread_pool_size ),
                                                                io_service_(),
                                                                acceptor_( io_service_ ),
                                                                signals_(io_service_)
{
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
  signals_.add(SIGSEGV);
  signals_.add(SIGILL);
  signals_.add(SIGFPE);
  signals_.add(SIGABRT);
#if defined(SIGBREAK)
  signals_.add(SIGBREAK);
#endif
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif

  signals_.async_wait(boost::bind(&http_server::stop, this));

  boost::asio::ip::tcp::resolver resolver( io_service_ );
  boost::asio::ip::tcp::resolver::query query( address, port );
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve( query );

  acceptor_.open( endpoint.protocol() );
  acceptor_.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true) );
  acceptor_.set_option( boost::asio::socket_base::keep_alive(false) );
  //acceptor_.set_option( boost::asio::socket_base::debug(true) );

  acceptor_.bind( endpoint );
  acceptor_.listen();

  std::clog << "\nhttp server waiting for connections at " << address
            << ":" << port << " (" << thread_pool_size << " threads)" << std::endl << std::flush;

  accept_new_connection();
}

void http_server::start() {
  std::vector< boost::shared_ptr<boost::thread> > threads;
  for ( std::size_t i = 0; i < thread_pool_size_; ++i ) {
    // these threads will block until async operations are complete
    // the first async operation is created by a call to AcceptNewConnection() in http_server::http_server()
    boost::shared_ptr<boost::thread> connection_thread( new boost::thread( boost::bind( &boost::asio::io_service::run, &io_service_) ) );
    threads.push_back( connection_thread );
  }

  for ( std::size_t i = 0; i < threads.size(); ++i ) {
    threads[i]->join();
  }
}

void http_server::accept_new_connection() {
  new_connection_.reset( new connection(io_service_) );
  acceptor_.async_accept( new_connection_->socket(),
                          boost::bind(&http_server::handle_connection, this, boost::asio::placeholders::error) );
}

void http_server::handle_connection(const boost::system::error_code& e) {
  // check if server was stopped by a signal before handle_connection()
  // had a chance to run
  if ( ! acceptor_.is_open() ) {
    std::cerr << "\nhandle_connection: acceptor_ closed prematurely" << std::flush;
    return;
  }

  if ( !e ) {
    new_connection_->process_connection();
  } else {
    std::cerr << "\nhandle_connection: was passed error: " << e.message() << std::flush;
  }
  accept_new_connection();
}

void http_server::stop() {
  std::cout << "\nDestructing vise_request_handler ..." << std::endl;
  delete vise::vise_request_handler::instance(); // leads to memory leak if not invoked

  std::cout << "\nDestructing search_engine_manager ..." << std::endl;
  delete vise::search_engine_manager::instance(); // leads to memory leak if not invoked

  std::cout << "\nStopping http server ..." << std::flush;
  //acceptor_.cancel();
  //acceptor_.close();
  io_service_.stop();
  std::cout << " [done]" << std::endl;
}
