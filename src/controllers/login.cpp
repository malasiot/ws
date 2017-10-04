#include <wspp/controllers/login.hpp>

#include <wspp/util/crypto.hpp>
#include <wspp/util/variant.hpp>

#include <wspp/views/forms.hpp>
#include <wspp/server/exceptions.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/make_shared.hpp>

#include <wspp/util/i18n.hpp>

using namespace std ;
using namespace wspp::util ;
using namespace wspp::server ;

namespace wspp { namespace web {

class LoginForm: public wspp::web::Form {
public:
    LoginForm(User &auth) ;

    bool validate(const Request &vals) override ;

    void onSuccess(const Request &request) override;

private:
    User &auth_ ;

};

LoginForm::LoginForm(User &auth): auth_(auth) {

    field<InputField>("username", "text").required().label(_("Username"))
        .setNormalizer([&] (const string &val) {
            return User::sanitizeUserName(val) ;
        })
        .addValidator<NonEmptyValidator>();

    field<InputField>("password", "password").required().label(_("Password"))
        .setNormalizer([&] (const string &val) {
            return User::sanitizePassword(val) ;
        })
        .addValidator<NonEmptyValidator>() ;

    field<InputField>("csrf_token", "hidden").initial(auth_.token()) ;

    field<CheckBoxField>("remember-me").label(_("Remember Me:")) ;
}

bool LoginForm::validate(const Request &vals) {
    if ( !Form::validate(vals) ) return false ;

    if ( !hashCompare(getValue("csrf_token"), auth_.token()) )
        throw std::runtime_error("Security exception" ) ;

    string username = getValue("username") ;
    string password = getValue("password") ;

    if ( !auth_.userNameExists(username) ) {
        errors_.push_back(_("Username does not exist")) ;
        return false ;
    }

    string stored_password, user_id, role ;
    auth_.load(username, user_id, stored_password, role) ;

    if ( !auth_.verifyPassword(password, stored_password) ) {
        errors_.push_back(_("Password mismatch")) ;
        return false ;
    }

    return true ;
}

void LoginForm::onSuccess(const Request &request) {
    string username = getValue("username") ;
    bool remember_me = getValue("remember-me") == "on" ;

    string stored_password, user_id, role ;
    auth_.load(username, user_id, stored_password, role) ;
    auth_.persist(username, user_id, role, remember_me) ;
}

bool LoginController::dispatch()
{
    if ( request_.matches("GET|POST", "/user/login/") ) login() ;
    else if ( request_.matches("POST", "/user/logout/") ) logout() ;
    else return false ;
    return true ;
}

void LoginController::login()
{
    LoginForm form(user_) ;

    form.handle(request_, response_, engine_) ;
}

void LoginController::logout()
{
    user_.forget() ;
    response_.writeJSON("{}");
}



} // namespace web
} // namespace wspp
