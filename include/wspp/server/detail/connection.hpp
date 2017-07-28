//
// HttpConnection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_CONNECTION_HPP
#define HTTP_SERVER_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <wspp/server/response.hpp>
#include <wspp/server/request.hpp>
#include <wspp/util/logger.hpp>

#include <wspp/server/request_handler.hpp>
#include <wspp/server/detail/request_parser.hpp>
#include <wspp/server/detail/connection_manager.hpp>

namespace wspp {
class ConnectionManager ;
class Server ;

extern std::vector<boost::asio::const_buffer> response_to_buffers(Response &rep, bool) ;

/// Represents a single HttpConnection from a client.

class Connection:
        public boost::enable_shared_from_this<Connection>
{
public:
    explicit Connection(boost::asio::ip::tcp::socket socket,
                        ConnectionManager& manager,
                        Logger &logger,
                        RequestHandler &handler) : socket_(std::move(socket)),
        connection_manager_(manager), handler_(handler), logger_(logger) {}

private:

    friend class wspp::Server ;
    friend class wspp::ConnectionManager ;



    void start() {
        read() ;
    }
    void stop() {
        socket_.close();
    }


    void read() {
        auto self(this->shared_from_this());
        socket_.async_read_some(boost::asio::buffer(buffer_), [self, this] (boost::system::error_code e, std::size_t bytes_transferred) {
            if (!e)
            {
                boost::tribool result;
                result = request_parser_.parse(buffer_.data(), bytes_transferred);

                if ( result )
                {
                    if ( !request_parser_.decode_message(request_) ) {
                        response_.stock_reply(Response::bad_request);
                    }
                    else {
                         try {
                             handler_.handle(request_, response_) ;

                             LOG_X_STREAM(logger_, Info, "Response to " <<
                                          socket_.remote_endpoint().address().to_string()
                                            << ": \"" << request_.method_ << " " << request_.path_
                                            << ((request_.query_.empty()) ? "" : "?" + request_.query_) << " "
                                            << request_.protocol_ << "\" "
                                            << response_.status_ << " " << response_.headers_.value<int>("Content-Length", 0)
                                          ) ;

                         }
                         catch ( ... ) {
                             response_.stock_reply(Response::internal_server_error);
                         }
                    }

                    write(response_to_buffers(response_, request_.method_ == "HEAD")) ;

                }
                else if (!result)
                {
                    response_.stock_reply(Response::bad_request);

                    write(response_to_buffers(response_, request_.method_ == "HEAD")) ;

                }
                else
                {
                    read() ;
                }
            }
            else if (e != boost::asio::error::operation_aborted)
            {
                connection_manager_.stop(self) ;
            }

        });
    }

    void write(const std::vector<boost::asio::const_buffer> &buffers)  {
        auto self(this->shared_from_this());
        boost::asio::async_write(socket_, buffers, [this, self](boost::system::error_code e, std::size_t) {
            if (!e)
            {
                // Initiate graceful Connection closure.
                boost::system::error_code ignored_ec;
                socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            }

            if (e != boost::asio::error::operation_aborted)
            {
                connection_manager_.stop(self) ;
            }
       });
    }

     boost::asio::ip::tcp::socket socket_;


     /// The handler of incoming HttpRequest.
     RequestHandler &handler_;

     /// Buffer for incoming data.
     boost::array<char, 8192> buffer_;

     ConnectionManager& connection_manager_ ;

      /// The parser for the incoming HttpRequest.
     detail::RequestParser request_parser_;

     Logger &logger_ ;

     Request request_ ;
     Response response_ ;

};


}
#endif // HTTP_SERVER_CONNECTION_HPP
