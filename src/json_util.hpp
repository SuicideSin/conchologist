#ifndef JSON_UTIL_HPP
#define JSON_UTIL_HPP

#include <jsoncpp/json.h>
#include <string>

std::string JSON_stringify(const Json::Value& json);

Json::Value JSON_parse(const std::string& str);

#endif