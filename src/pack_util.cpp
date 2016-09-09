#include "pack_util.hpp"

#include <msl/crypto.hpp>
#include <msl/string.hpp>

std::string pack_hex(std::string ascii)
{
	return msl::to_hex_string(ascii)+"\n";
}

std::string unpack_hex(std::string hex)
{
	while(msl::starts_with(hex,"\n"))
		hex=hex.substr(1,hex.size()-1);
	while(msl::ends_with(hex,"\n"))
		hex=hex.substr(0,hex.size()-1);
	hex=msl::from_hex_string(hex);
	return hex;
}

std::string pack_rsa(const std::string& plain,const std::string& key)
{
	return pack_hex(msl::encrypt_rsa(plain,key));
}

std::string unpack_rsa(const std::string& cipher,const std::string& key)
{
	return msl::decrypt_rsa(unpack_hex(cipher),key);
}

std::string pack_aes(const std::string& plain,const std::string& key,const std::string& iv)
{
	return pack_hex(msl::encrypt_aes256(plain,key,iv));
}

std::string unpack_aes(const std::string& cipher,const std::string& key,const std::string& iv)
{
	return msl::decrypt_aes256(unpack_hex(cipher),key,iv);
}