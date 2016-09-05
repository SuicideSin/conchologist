#include "pack_util.hpp"

#include <msl/crypto.hpp>
#include <msl/string.hpp>

std::string pack_rsa(const std::string& plain,const std::string& key)
{
	std::string cipher=msl::encrypt_rsa(plain,key);
	cipher=msl::to_hex_string(cipher)+"\n";
	return cipher;
}

std::string unpack_rsa(const std::string& cipher,const std::string& key)
{
	std::string plain=cipher;
	while(msl::starts_with(plain,"\n"))
		plain=plain.substr(1,plain.size()-1);
	while(msl::ends_with(plain,"\n"))
		plain=plain.substr(0,plain.size()-1);
	plain=msl::from_hex_string(plain);
	plain=msl::decrypt_rsa(plain,key);
	return plain;
}

std::string pack_aes(const std::string& plain,const std::string& key,const std::string& iv)
{
	std::string cipher=msl::encrypt_aes256(plain,key,iv);
	cipher=msl::to_hex_string(cipher)+"\n";
	return cipher;
}

std::string unpack_aes(const std::string& cipher,const std::string& key,const std::string& iv)
{
	std::string plain=cipher;
	while(msl::starts_with(plain,"\n"))
		plain=plain.substr(1,plain.size()-1);
	while(msl::ends_with(plain,"\n"))
		plain=plain.substr(0,plain.size()-1);
	plain=msl::from_hex_string(plain);
	plain=msl::decrypt_aes256(plain,key,iv);
	return plain;
}