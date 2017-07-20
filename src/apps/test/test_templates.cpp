#include <string>
#include <sstream>
#include <iostream>

#include <wspp/server/request_handler.hpp>
#include <wspp/server/response.hpp>
#include <wspp/server/request.hpp>
#include <wspp/server/session_manager.hpp>
#include <wspp/server/session.hpp>
#include <wspp/server/server.hpp>
#include <wspp/server/router.hpp>

#include <wspp/util/logger.hpp>

#include <iostream>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

using namespace std ;
using namespace wspp ;

class MyHandler: public RequestHandler {

public:
    MyHandler(): RequestHandler() {}

    virtual bool handle(Request& req, Response& resp, SessionManager &sm) {

        // test if the request path is what expected

        string user ;
        if ( !req.matches("GET", R"(/hello/([a-zA-Z]+))", user) ) return false ;

        Session session ;
        sm.open(req, session) ;

        // all rendering is done in there
        render(resp, user) ;

        resp.setContentLength() ;
        resp.setContentType("text/html") ;

        session.data_["user_name"] = user ;

        resp.setStatus(Response::ok) ;

        sm.close(resp, session) ;

        return true ;
    }

    void render(Response &response, const string &key) {
#include "test_app/templates/test.tpp"
    }
};


class DefaultLogger: public Logger
{
public:
    DefaultLogger(const std::string &log_file, bool debug) {
        if ( debug ) addAppender(std::make_shared<LogStreamAppender>(Trace, make_shared<LogPatternFormatter>("%In function %c, %F:%l: %m"), std::cerr)) ;
        if ( boost::filesystem::exists(log_file) )
            addAppender(std::make_shared<LogFileAppender>(Info, make_shared<LogPatternFormatter>("%V: %d %r: %m"), log_file)) ;
    }
};

std::unique_ptr<DefaultLogger> g_server_logger ;

Logger &get_current_logger() {
    return *g_server_logger ;
}

int main(int argc, char *argv[]) {

    g_server_logger.reset(new DefaultLogger("/tmp/server-log", true)) ;

    MemSessionManager sm ;

    std::shared_ptr<Router> router(new Router()) ;
    router->addRoute({"GET"}, "/delete/{id:n}/", std::make_shared<MyHandler>() ) ;

    Server server(router, "127.0.0.1", "5000", sm, 10) ;

    server.run() ;
}
