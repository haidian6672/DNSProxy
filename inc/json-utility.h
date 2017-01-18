#pragma once

#ifndef __DNS_JSON_UTILITY_H__
#define __DNS_JSON_UTILITY_H__

#include <list>
#include <algorithm>

#include "json/json.h"
#include "json/json-forwards.h"

using namespace std;

// Json Utility
const Json::Value& check_key_and_get_value(const Json::Value& node, const string &key);
const Json::Value& check_key_mustbe_array(const Json::Value& node, const string &key );
const Json::Value& check_key_with_default(const Json::Value& node, const string &key, const Json::Value &defaultValue);
void check_json_value_mustby_object(const Json::Value &node);

#endif

/*
 Push Chen.
 littlepush@gmail.com
 http://pushchen.com
 http://twitter.com/littlepush
 */
