#include <wspp/views/forms.hpp>
#include <wspp/util/crypto.hpp>

#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>

using namespace std ;
using namespace wspp::server ;
using namespace wspp::util ;

namespace wspp { namespace web {


string FormField::getAlias() const
{
    if ( !alias_.empty() ) return alias_ ;
    else if ( !label_.empty() ) {
        return boost::to_lower_copy(label_) ;
    }
    else
        return boost::to_lower_copy(name_) ;
}

void FormField::fillData(Variant::Object &res) const {

    if ( !label_.empty() ) res.insert({"label", label_}) ;
    if ( !name_.empty() ) res.insert({"name", name_}) ;

    if ( !value_.empty() ) res.insert({"value", value_}) ;
    else if ( !initial_value_.empty() ) res.insert({"value", initial_value_}) ;

    res.insert({"id", id_}) ;

    if ( !place_holder_.empty() ) res.insert({"placeholder", place_holder_}) ;
    if ( !help_text_.empty() ) res.insert({"help_text", help_text_}) ;
    res.insert({"required", required_}) ;
    res.insert({"disabled", disabled_}) ;
    if ( !extra_classes_.empty() ) res.insert({"extra_classes", extra_classes_}) ;
    if ( !extra_attrs_.empty() ) res.insert({"extra_attrs", Variant::fromDictionary(extra_attrs_)}) ;
    if ( !error_messages_.empty() ) res.insert({"errors", Variant::Object{{"messages", Variant::fromVector(error_messages_)}}}) ;
}

bool FormField::validate(const string &value)
{
    // normalize passed value

    string n_value = normalizer_ ? normalizer_(value) : value ;

    // call validators

    for( auto &v: validators_ ) {
        try {
            v->validate(n_value, *this) ;
        }
        catch ( FormFieldValidationError &e ) {
            addErrorMsg(e.what());
            return false ;
        }
    }

    // store value

    value_ = n_value ;
    is_valid_ = true ;
    return true ;
}

void InputField::fillData(Variant::Object &base) const
{
    FormField::fillData(base) ;
    base.insert({"type", type_}) ;
    base.insert({"widget", "input-field"}) ;
}


SelectField::SelectField(const string &name, boost::shared_ptr<OptionsModel> options, bool multi): FormField(name), options_(options), multiple_(multi) {
    addValidator([&](const string &val, const FormField &) {
        Dictionary options = options_->fetch() ;
        if ( multiple_ ) {
            boost::char_separator<char> sep(" ");
            boost::tokenizer<boost::char_separator<char> > tokens(val, sep);

            for( const string &s: tokens ) {
                if ( !options.contains(s) ) {
                    throw FormFieldValidationError("Supplied value: " + val + " is not in option list") ;
                }
            }
        }
        else {
            if ( !options.contains(val) ) {
                throw FormFieldValidationError("Supplied value: " + val + " is not in option list") ;
            }
        }
    }) ;

}

void SelectField::fillData(Variant::Object &base) const
{
    FormField::fillData(base) ;
    base.insert({"multiselect", multiple_}) ;
    base.insert({"widget", "select-field"}) ;
    if ( options_ ) {
        Dictionary options = options_->fetch() ;
        if ( !options.empty() ) base.insert({"options", Variant::fromDictionaryAsArray(options)}) ;
    }
}


CheckBoxField::CheckBoxField(const string &name, bool is_checked): FormField(name), is_checked_(is_checked) {

}

void CheckBoxField::fillData(Variant::Object &res) const {
    FormField::fillData(res) ;
    if ( is_checked_ ) res.insert({"checked", "checked"}) ;
    res.insert({"template", "checkbox"}) ;
}
/*
CSRFField::CSRFField(const string &name,  Session &session): InputField(name, "hidden"), con_(con), session_(session)
{
    string selector = encodeBase64(randomBytes(12)) ;
    string token = randomBytes(24) ;
    time_t expires = std::time(nullptr) + 3600*1; // Expire in 1 hour

    sqlite::Statement stmt(con_, "INSERT INTO auth_tokens ( user_id, selector, token, expires ) VALUES ( ?, ?, ?, ? );", user_id, selector, binToHex(hashSHA256(token)), expires) ;
    stmt.exec() ;

        response_.setCookie("auth_token", selector + ":" + encodeBase64(token),  expires, "/");
    }

    // update user info
    sqlite::Statement stmt(con_, "UPDATE user_info SET last_sign_in = ? WHERE user_id = ?", std::time(nullptr), user_id) ;
    stmt.exec() ;

}

}
*/


Form::Form(const std::string &prefix, const string &suffix): field_prefix_(prefix), field_suffix_(suffix) {

}

void Form::addField(const FormField::Ptr &field)
{
    field->id(field_prefix_ + field->name_ + field_suffix_) ;
    field->count_ = fields_.size() ;
    fields_.push_back(field) ;
    field_map_.insert({field->name_, field}) ;
}


Variant::Object Form::data() const
{
    Variant::Object form_data ;

    if ( !errors_.empty() ) {
        Variant::Array errors ;
        for( const string &msg: errors_ )
            errors.emplace_back(Variant::Object{{"message", msg}}) ;
        form_data.insert({"global-errors", errors}) ;
    }

    Variant::Array field_data_list ;
    for( const auto &p: fields_ ) {
        Variant::Object field_data ;
        p->fillData(field_data) ;
        if ( is_valid_ )
            field_data.insert({"value", p->value_}) ;
        else if ( !p->initial_value_.empty() )
            field_data.insert({"value", p->initial_value_}) ;

        field_data_list.push_back(field_data) ;
    }

    form_data.insert({"fields", field_data_list}) ;

    return form_data ;
}

string Form::getValue(const string &field_name)
{
    const auto &it = field_map_.find(field_name) ;
    assert ( it != field_map_.end() ) ;
    return it->second->value_ ;
}

bool Form::validate(const Dictionary &vals) {
    bool failed = false ;
    for( const auto &p: fields_ ) {
        if ( vals.contains(p->name_) ) {

            bool res = p->validate(vals.get(p->name_)) ;
            if ( !res ) failed = true ;
        }
    }
    is_valid_ = !failed ;
    return is_valid_ ;
}

void Form::init(const Dictionary &vals) {
    for( const auto &p: vals ) {
        auto it = field_map_.find(p.first) ;
        if ( it != field_map_.end() ) {
            it->second->value(p.second) ;
        }
    }
}


} // namespace web
} // namespace wspp
