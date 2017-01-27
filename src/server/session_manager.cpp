#include "session_manager.hpp"
#include "session.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

using namespace std ;

namespace http {

void SessionManager::open(const Request &req, Session &session_data)
{
    session_data.id_ = req.COOKIE_.get("WSX_SESSION_ID") ;

    if ( session_data.id_.empty() ) {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        session_data.id_ = boost::lexical_cast<std::string>(uuid) ;
    }

    load(session_data) ;
}

void SessionManager::close(Response &resp, const Session &session_data) {

    save(session_data) ;
    resp.headers_.add("Set-Cookie", "WSX_SESSION_ID=" + session_data.id_) ;
}


void MemSessionManager::save(const Session &session)  { data_[session.id_] = session.data_ ;}
void MemSessionManager::load(Session &session) { session.data_ = data_[session.id_] ; }



}
