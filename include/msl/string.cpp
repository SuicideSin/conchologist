#include "string.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

std::string msl::to_lower(std::string str)
{
	std::transform(str.begin(),str.end(),str.begin(),tolower);
	return str;
}

std::string msl::to_upper(std::string str)
{
	std::transform(str.begin(),str.end(),str.begin(),toupper);
	return str;
}

std::string msl::to_hex_string(const std::string& str)
{
	std::ostringstream ostr;

	for(size_t ii=0;ii<str.size();++ii)
		ostr<<std::hex<<std::setw(2)<<std::setfill('0')<<((int)str[ii]&0x000000ff);

	return ostr.str();
}

std::string msl::from_hex_string(std::string str)
{
	std::string ret="";

	if(str.size()%2!=0)
		str.insert(0,"0");

	str=msl::to_lower(str);

	size_t pos=0;
	std::string temp_byte;

	while(pos<str.size())
	{
		if(!((str[pos]>='0'&&str[pos]<='9')||(str[pos]>='a'&&str[pos]<='f')))
			throw std::runtime_error("msl::from_hex_string() - Non-hex character in string.");

		temp_byte+=str[pos];

		if(temp_byte.size()>=2||pos+1==str.size())
		{
			std::istringstream istr("0x"+temp_byte);
			int real_byte;
			if(!(istr>>std::hex>>real_byte))
				throw std::runtime_error("msl::from_hex_string() - String conversion error.");
			ret+=real_byte;
			temp_byte="";
		}

		++pos;
	}

	return ret;
}

std::string msl::replace_all(std::string str,const std::string& find,const std::string& replace)
{
	size_t pos=0;

	while((pos=str.find(find,pos))!=std::string::npos)
	{
		str.replace(pos,find.size(),replace);
		pos+=replace.size();
	}

	return str;
}

bool msl::ends_with(const std::string& str,const std::string& suffix)
{
	return (str.size()>=suffix.size()&&starts_with(str.substr(str.size()-suffix.size(),suffix.size()),suffix));
}

bool msl::starts_with(const std::string& str,const std::string& prefix)
{
	return (str.size()>=prefix.size()&&str.compare(0,prefix.size(),prefix)==0);
}
