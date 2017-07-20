//
// Connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <wspp/server/detail/connection.hpp>

#include <wspp/server/detail/request_handler_factory.hpp>
#include <wspp/server/detail/connection_manager.hpp>
#include <wspp/server/session_manager.hpp>

#include <vector>
#include <boost/bind.hpp>

namespace wspp {

Connection::Connection(boost::asio::ip::tcp::socket socket,
                       ConnectionManager& manager,
                       SessionManager &sm,
                       const std::shared_ptr<RequestHandler>& handler)
    : socket_(std::move(socket)),
      connection_manager_(manager),
      session_manager_(sm),
      handler_(handler)
{
}

boost::asio::ip::tcp::socket &Connection::socket()
{
    return socket_;
}

void Connection::start()
{
    socket_.async_read_some(boost::asio::buffer(buffer_),
                            boost::bind(&Connection::handle_read, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void Connection::stop() {
    socket_.close();
}

extern std::vector<boost::asio::const_buffer> response_to_buffers(const Response &rep) ;

void Connection::handle_read(const boost::system::error_code& e,
                             std::size_t bytes_transferred)
{

    if (!e)
    {
        boost::tribool result;
        result = request_parser_.parse(buffer_.data(), bytes_transferred);

        if ( result )
        {
            if ( !request_parser_.decode_message(request_) ) {
                reply_ = Response::stock_reply(Response::bad_request);
            }
            else {

                if ( handler_ ) {

                    try {
                        if ( !handler_->handle(request_, reply_, session_manager_ ) )
                            reply_ = Response::stock_reply(Response::not_found);

                    }
                    catch ( ... ) {
                        reply_ = Response::stock_reply(Response::internal_server_error);
                    }

                }
                else
                    reply_ = Response::stock_reply(Response::not_found);

            }

            boost::asio::async_write(socket_, response_to_buffers(reply_),
                                     boost::bind(&Connection::handle_write, shared_from_this(),
                                     boost::asio::placeholders::error));

        }
        else if (!result)
        {
            reply_ = Response::stock_reply(Response::bad_request);
            boost::asio::async_write(socket_, response_to_buffers(reply_),
                                     boost::bind(&Connection::handle_write, shared_from_this(),
                                     boost::asio::placeholders::error));
        }
        else
        {
            socket_.async_read_some(boost::asio::buffer(buffer_),
                                    boost::bind(&Connection::handle_read, shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
    }
    else if (e != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}

void Connection::handle_write(const boost::system::error_code& e)
{
    if (!e)
    {
        // Initiate graceful Connection closure.
        boost::system::error_code ignored_ec;
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
    }

    if (e != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}


} // namespace http
