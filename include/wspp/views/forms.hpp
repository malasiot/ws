#ifndef __WSPP_UTIL_FORMS_HPP__
#define __WSPP_UTIL_FORMS_HPP__

#include <string>
#include <wspp/util/dictionary.hpp>
#include <wspp/util/variant.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

using std::string ;

namespace wspp { namespace web {

// Form helper class. The aim of the class is:
// 1) declare form in a view agnostic way
// 2) validate input data for those fields e.g passed in a POST request
// 3) pack data needed to render the form into an object that can be passed to the template engine

using wspp::util::Variant ;
using wspp::util::Dictionary ;

class FormField {
public:
    // the validator checks the validity of the input string.
    // if invalid then it should fill the error_messages_ of the field
    typedef boost::function<bool (const string &, FormField &)> Validator ;

    // The normalizer preprocesses an input value before passing it to validators
    typedef boost::function<string (const string &)> Normalizer ;

    static Validator requiredArgValidator ;

    typedef boost::shared_ptr<FormField> Ptr ;

public:

    FormField(const string &name): name_(name) {
        validators_.push_back(requiredArgValidator) ;
    }

    // set field to required
    void required(bool is_required = true) { required_ = is_required ; }
    // set field to disabled
    void disabled(bool is_disabled = true) { disabled_ = is_disabled ; }
    // set field value
    void value(const string &val) { value_ = val ; }
    // append classes to class attribute
    void appendClass(const string &extra) { extra_classes_ = extra ;  }
    // append extra attributes to element
    void extraAttributes(const Dictionary &attrs) { extra_attrs_ = attrs ; }
    // set custom validator
    void addValidator(Validator val) { validators_.push_back(val) ; }
    // set custom normalizer
    void setNormalizer(Normalizer val) { normalizer_ = val ; }
    // set field id
    void id(const string &id) { id_ = id ; }
    // set label
    void label(const string &label) { label_ = label ; }
    // set placeholder
    void placeholder(const string &p) { place_holder_ = p ; }
    // set initial value
    void initial(const string &v) { initial_value_ = v ; }
    // set help text
    void help(const string &text) { help_text_ = text ; }

    void addErrorMsg(const string &msg) { error_messages_.push_back(msg) ; }

protected:
    virtual void fillData(Variant::Object &) const ;
    // calls all validators
    virtual bool validate(const string &value) ;

private:
    friend class Form ;

    string label_, name_, value_, id_, place_holder_, initial_value_, help_text_ ;
    bool required_ = false, disabled_ = false ;
    string extra_classes_ ;
    Dictionary extra_attrs_ ;
    std::vector<string> error_messages_ ;
    std::vector<Validator> validators_ ;
    Normalizer normalizer_ ;
    uint count_ = 0 ;

};

// This is used to abstract a data source which provides key value pairs to be used e.g. in selection boxes or radio boxes
// e.g. when options have to be loaded from a database or file

class OptionsModel {
public:
    typedef boost::shared_ptr<OptionsModel> Ptr ;

    virtual Dictionary fetch() = 0 ;
};

// convenience class for wrapping a dictionary

class DictionaryOptionsModel: public OptionsModel {
public:
    DictionaryOptionsModel(const Dictionary &dict): dict_(dict) {}
    virtual Dictionary fetch() override {
        return dict_ ;
    }

private:
    Dictionary dict_ ;
};

// wrapper for a lambda
class CallbackOptionsModel: public OptionsModel {
public:
    CallbackOptionsModel(boost::function<Dictionary ()> cb): cb_(cb) {}

    Dictionary fetch() override { return cb_() ; }
private:
    boost::function<Dictionary ()> cb_ ;
};


class InputField: public FormField {
public:
    InputField(const string &name, const string &type): FormField(name), type_(type) {}

    void fillData(Variant::Object &) const override;
private:
    string type_ ;
};

class SelectField: public FormField {
public:
    SelectField(const string &name, boost::shared_ptr<OptionsModel> options, bool multi = false);

    void fillData(Variant::Object &) const override;

private:
    bool multiple_ ;
    boost::shared_ptr<OptionsModel> options_ ;
};

class CheckBoxField: public FormField {
public:
    CheckBoxField(const string &name, bool is_checked = false);

    void fillData(Variant::Object &res) const override;
private:
    bool is_checked_ = false ;
};

class Form {
public:

    Form(const string &field_prefix = "", const string &field_suffix = "_field") ;

    void addField(const FormField::Ptr &field) ;

    // add an input field
    FormField &input(const string &name, const string &type) ;

    // add a select field
    FormField &select(const string &name, boost::shared_ptr<OptionsModel> options, bool multi = false) ;

    // add a checkbox
    FormField &checkbox(const string &name, bool is_checked = false) ;

    // call to validate the user data against the form
    // the field values are stored in case of succesfull field validation
    // override to add additional validation e.g. requiring more than one fields (do not forget to call base class)
    virtual bool validate(const Dictionary &vals) ;

    // init values with user supplied (no validation)
    void init(const Dictionary &vals) ;

    Variant::Object data() const ;

    string getValue(const string &field_name) ;

protected:

    std::vector<FormField::Ptr> fields_ ;
    std::map<string, FormField::Ptr> field_map_ ;
    string field_prefix_, field_suffix_ ;
    std::vector<string> errors_ ;
    bool is_valid_ = false ;
} ;


} // namespace web
} // namespace wspp

#endif
