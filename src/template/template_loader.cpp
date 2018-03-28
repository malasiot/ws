#include "template_loader.hpp"

#include <fstream>
#include <sstream>

#include <boost/filesystem.hpp>

using namespace std ;

FileSystemTemplateLoader::FileSystemTemplateLoader(const std::initializer_list<string> &root_folders, const string &suffix):
    root_folders_(root_folders), suffix_(suffix) {
}

string FileSystemTemplateLoader::load(const string &key) {

    using namespace boost::filesystem ;

    for ( const string &r: root_folders_ ) {
        path p(r) ;

        p /= key + suffix_;

        if ( !exists(p) ) continue ;

        ifstream in(p.string()) ;

        return static_cast<stringstream const&>(stringstream() << in.rdbuf()).str() ;
    }

    return string() ;
}