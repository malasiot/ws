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

#include <wspp/server/request_handler.hpp>
#include <wspp/server/detail/request_parser.hpp>
#include <wspp/server/session_manager.hpp>
#include <wspp/server/detail/connection_manager.hpp>

namespace wspp {
class ConnectionManager ;

extern std::vector<boost::asio::const_buffer> response_to_buffers(const Response &rep) ;

/// Represents a single HttpConnection from a client.

class Connection:
        public boost::enable_shared_from_this<Connection>
{
public:

    /// Construct a HttpConnection with the given io_service.
    explicit Connection(boost::asio::ip::tcp::socket socket,
                        ConnectionManager& manager,  SessionManager &sm,
                        boost::shared_ptr<RequestHandler> handler) : socket_(std::move(socket)),
        connection_manager_(manager), session_manager_(sm), handler_(handler) {}


    void start() {
        read() ;
    }
    void stop() {
        socket_.close();
    }

private:

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
                        reply_.stock_reply(Response::bad_request);
                    }
                    else {

                        if ( handler_ ) {

                            Session session ;
                            session_manager_.open(request_, session) ;

                            try {
                                handler_->handle(request_, reply_, session) ;
                                session_manager_.close(reply_, session) ;
                            }
                            catch ( ... ) {
                                reply_.stock_reply(Response::internal_server_error);
                            }

                        }
                        else
                            reply_.stock_reply(Response::not_found);

                    }

                    write(response_to_buffers(reply_)) ;

                }
                else if (!result)
                {
                    reply_.stock_reply(Response::bad_request);

                    write(response_to_buffers(reply_)) ;

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
     boost::shared_ptr<RequestHandler> handler_;

     /// Buffer for incoming data.
     boost::array<char, 8192> buffer_;

     ConnectionManager& connection_manager_ ;

     SessionManager &session_manager_ ;

     /// The incoming HttpRequest.
     Request request_;

     /// The parser for the incoming HttpRequest.
     detail::RequestParser request_parser_;

     /// The reply to be sent back to the client.
     Response reply_;
};


}
#endif // HTTP_SERVER_CONNECTION_HPP
