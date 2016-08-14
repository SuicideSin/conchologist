#include "json_util.hpp"

#include <stdexcept>
#include <sstream>

std::string JSON_stringify(const Json::Value& json)
{
	Json::FastWriter writer;
	std::string ret=writer.write(json);
	while(ret.size()>0&&isspace(ret[ret.size()-1])!=0)
		ret=ret.substr(0,ret.size()-1);
	return ret;
}

Json::Value JSON_parse(const std::string& str)
{
	Json::Reader reader;
	Json::Value json;
	if(!reader.parse(str,json))
		throw std::runtime_error(reader.getFormattedErrorMessages());
	return json;
}