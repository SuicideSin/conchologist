#ifndef PACK_UTIL_HPP
#define PACK_UTIL_HPP

#include <string>

std::string pack_hex(std::string ascii);
std::string unpack_hex(std::string hex);
std::string pack_rsa(const std::string& plain,const std::string& key);
std::string unpack_rsa(const std::string& cipher,const std::string& key);
std::string pack_aes(const std::string& plain,const std::string& key,const std::string& iv);
std::string unpack_aes(const std::string& cipher,const std::string& key,const std::string& iv);

#endif