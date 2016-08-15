#include "cc_handler.hpp"
#include <iostream>
#include <jsoncpp/json.h>
#include <json_util.hpp>
#include <stdexcept>
#include <string>
#include "web_handler.hpp"

void add_cb(cc_handler_t& handler,const cc_client_t& client);
void remove_cb(cc_handler_t& handler,const cc_client_t& client);
void recv_cb(cc_handler_t& handler,const cc_client_t& client);
bool web_cb(web_handler_t& handler,const web_client_t& client);

cc_handler_t cc_handler(add_cb,remove_cb,recv_cb);
web_handler_t web_handler("web",web_cb);

void add_cb(cc_handler_t& handler,const cc_client_t& client)
{
	std::cout<<"Added "<<client.address<<std::endl;
}

void remove_cb(cc_handler_t& handler,const cc_client_t& client)
{
	std::cout<<"Removed "<<client.address<<std::endl;
}

void recv_cb(cc_handler_t& handler,const cc_client_t& client)
{
	std::cout<<"Received "<<client.address<<" "<<client.history[client.history.size()-1]<<std::endl;
}

bool web_cb(web_handler_t& handler,const web_client_t& client)
{
	std::cout<<"Connection: "<<client.address<<" "<<
		client.method<<" "<<client.request;
	if(client.query.size()>0)
		std::cout<<"?"<<client.query;
	std::cout<<std::endl;
	if(client.method=="POST")
	{
		Json::Value obj;
		std::string response="200 OK";
		try
		{
			Json::Value request(JSON_parse(client.post_data));
			if(request["method"]=="updates")
			{
				Json::Value& counts=request["params"];
				Json::Value updates(Json::objectValue);
				cc_client_map_t clients=cc_handler.map();
				for(cc_client_map_t::const_iterator ii=clients.begin();ii!=clients.end();++ii)
				{
					std::string address=ii->second.address;
					const std::vector<std::string>& history=ii->second.history;

					if(!counts.isMember(address))
						counts[address]=0;

					size_t line_size=counts[address].asUInt();
					Json::Value new_lines(Json::arrayValue);
					for(size_t jj=line_size;jj<history.size();++jj)
						new_lines.append(history[jj]);
					updates[address]=new_lines;
				}
				obj["result"]=updates;
			}
			else if(request["method"]=="write")
			{
				std::string address=request["params"]["address"].asString();
				std::string line=request["params"]["line"].asString();
				cc_handler.send(address,line);
			}
			else
			{
				obj["error"]="Unsupported method.";
			}
		}
		catch(std::exception& error)
		{
			obj["error"]=error.what();
		}
		catch(...)
		{
			obj["error"]="Unknown error.";
		}
		std::cout<<"SENDING: "<<JSON_stringify(obj)<<std::endl;
		handler.send(client,response,JSON_stringify(obj));
		return true;
	}
	return false;
}

int main()
{
	try
	{
		cc_handler.connect("127.0.0.1:8080");
		std::cout<<"Started cc server on 127.0.0.1:8080"<<std::endl;
		web_handler.connect("127.0.0.1:8081");
		std::cout<<"Started web server on 127.0.0.1:8081"<<std::endl;
		while(true)
		{
			cc_handler.update();
			web_handler.update();
		}
	}
	catch(std::exception& error)
	{
		std::cout<<"Error - "<<error.what()<<std::endl;
		return 1;
	}
	catch(...)
	{
		std::cout<<"Unknown error occurred."<<std::endl;
		return 1;
	}

	return 0;
}