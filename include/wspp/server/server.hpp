//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef __WSPP_SERVER_HPP__
#define __WSPP_SERVER_HPP__

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <wspp/util/logger.hpp>
#include <wspp/server/detail/connection.hpp>
#include <wspp/server/detail/io_service_pool.hpp>
#include <wspp/server/detail/connection_manager.hpp>


namespace wspp { namespace server {

/// The top-level class of the HTTP server.
///
class Server: public RequestHandler
{
public:
    /// Construct the server to listen on the specified TCP address and port, and
    /// serve up files from the given directory.
    explicit Server(const std::string& address, const std::string& port,
                    std::size_t io_service_pool_size = 4);


    /// Run the server's io_service loop.
    void run();

    /// Stop server loop
    void stop() ;

    std::string getIPAddress() ;

private:
    /// Initiate an asynchronous accept operation.
    void start_accept();

    /// Handle completion of an asynchronous accept operation.
    void handle_accept(const boost::system::error_code& e);

    /// Handle a request to stop the server.
    void handle_stop();

    void do_await_stop() ;

    /// The pool of io_service objects used to perform asynchronous operations.
    detail::io_service_pool io_service_pool_;

    /// The signal_set is used to register for process termination notifications.
    boost::asio::signal_set signals_;

    /// Acceptor used to listen for incoming connections.
    boost::asio::ip::tcp::acceptor acceptor_;

    ConnectionManager connection_manager_;

     /// The next socket to be accepted.
    boost::asio::ip::tcp::socket socket_;
};

} // namespace server
} // namespace wspp

#endif
