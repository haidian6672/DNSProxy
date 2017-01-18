#include "json-utility.h"

// Json Utility
const Json::Value& check_key_and_get_value(const Json::Value& node, const string &key) {
    if ( node.isMember(key) == false ) {
        ostringstream _oss;
        _oss << "missing \"" << key << "\"" << endl;
        Json::FastWriter _jsonWriter;
        _oss << "check on config node: " << _jsonWriter.write(node) << endl;
        throw( runtime_error(_oss.str()) );
    }
    return node[key];
}

const Json::Value& check_key_mustbe_array(
    const Json::Value& node, 
    const string &key ) {
    bool _is_type = node[key].isArray();
    if ( !_is_type ) {
        ostringstream _oss;
        _oss << "checking array for key: \"" << key << "\" failed." << endl;
        Json::FastWriter _jsonWriter;
        _oss << "node is: " << _jsonWriter.write(node) << endl;
        throw( runtime_error(_oss.str()) );
    }
    return node[key];
}

const Json::Value& check_key_with_default(
    const Json::Value& node, 
    const string &key, 
    const Json::Value &defaultValue) {
    if ( node.isMember(key) == false ) return defaultValue;
    return node[key];
}

void check_json_value_mustby_object(const Json::Value &node) {
    if ( node.isObject() ) return;
    ostringstream _oss;
    Json::FastWriter _jsonWriter;
    _oss << "checking object for node: " << endl << "\t" <<
        _jsonWriter.write(node) << endl << "\033[1;31mFailed\033[0m" << endl;
    throw( runtime_error(_oss.str()) );
}
