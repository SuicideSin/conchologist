#include "rev_handler.hpp"
#include <iostream>
#include <jsoncpp/json.h>
#include <json_util.hpp>
#include <mongoose/mongoose.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "web_handler.hpp"

void add_cb(rev_handler_t& handler,const rev_client_t& client);
void remove_cb(rev_handler_t& handler,const rev_client_t& client);
void recv_cb(rev_handler_t& handler,const rev_client_t& client);
bool web_cb(web_handler_t& handler,web_client_t& client);
void web_close_cb(web_handler_t& handler,mg_connection* conn);
bool service_comet(web_client_t& client,const bool forced=false);
void service_comets(const bool forced=false);

rev_handler_t rev_handler(add_cb,remove_cb,recv_cb);
web_handler_t web_handler("web",web_cb,web_close_cb);
std::vector<web_client_t> comets;

void add_cb(rev_handler_t& handler,const rev_client_t& client)
{
	std::cout<<"Added "<<client.address<<std::endl;
	service_comets();
}

void remove_cb(rev_handler_t& handler,const rev_client_t& client)
{
	std::cout<<"Removed "<<client.address<<std::endl;
	service_comets(true);
}

void recv_cb(rev_handler_t& handler,const rev_client_t& client)
{
	std::cout<<"Received "<<client.address<<std::endl;
	service_comets();
}

bool web_cb(web_handler_t& handler,web_client_t& client)
{
	std::cout<<"Connection: "<<client.address<<" "<<
		client.method<<" "<<client.request;
	if(client.query.size()>0)
		std::cout<<"?"<<client.query;
	std::cout<<std::endl;
	bool send=true;
	if(client.method=="POST")
	{
		Json::Value obj;
		std::string response="200 OK";
		try
		{
			Json::Value request(JSON_parse(client.post_data));
			if(request["method"]=="updates")
			{
				if(!service_comet(client))
				{
					send=false;
					comets.push_back(client);
				}
			}
			else if(request["method"]=="kill")
			{
				std::string address(request["params"]["address"].asString());
				rev_handler.kill(address);
			}
			else if(request["method"]=="write")
			{
				std::string address(request["params"]["address"].asString());
				std::string line(request["params"]["line"].asString());
				rev_handler.send(address,line);
				service_comets();
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
		if(send)
			web_handler.send(client,response,JSON_stringify(obj));
		return true;
	}
	return false;
}

void web_close_cb(web_handler_t& handler,mg_connection* conn)
{
	std::vector<web_client_t> new_comets;
	for(size_t ii=0;ii<comets.size();++ii)
	{
		if(comets[ii].conn==conn)
		{
			comets[ii].conn->flags&=~MG_F_USER_1;
			handler.send(comets[ii],"408 Request Timeout","");
			continue;
		}
		new_comets.push_back(comets[ii]);
	}
	comets=new_comets;
}

bool service_comet(web_client_t& client,const bool forced)
{
	bool changed=false;
	Json::Value obj;
	std::string response="200 OK";
	Json::Value request(JSON_parse(client.post_data));
	Json::Value& counts=request["params"];
	Json::Value updates(Json::objectValue);
	rev_client_map_t rev_clients=rev_handler.map();

	for(rev_client_map_t::const_iterator ii=rev_clients.begin();ii!=rev_clients.end();++ii)
	{
		if(!ii->second.alive)
			continue;

		std::string address=ii->second.address;
		const std::vector<std::string>& history=ii->second.history;

		if(!counts.isMember(address))
		{
			counts[address]=0;
			changed=true;
		}

		size_t line_size=counts[address].asUInt();
		Json::Value new_lines(Json::arrayValue);
		for(size_t jj=line_size;jj<history.size();++jj)
		{
			new_lines.append(history[jj]);
			changed=true;
		}
		updates[address]["last_count"]=(Json::LargestUInt(line_size));
		updates[address]["new_lines"]=new_lines;
	}
	obj["result"]=updates;

	if(changed||forced)
	{
		client.conn->flags&=~MG_F_USER_1;
		web_handler.send(client,response,JSON_stringify(obj));
	}
	return (changed||forced);
}

void service_comets(const bool forced)
{
	std::vector<web_client_t> new_comets;
	for(size_t ii=0;ii<comets.size();++ii)
		if(!service_comet(comets[ii],forced))
			new_comets.push_back(comets[ii]);
	comets=new_comets;
}

int main()
{
	try
	{
		std::string rev_address("0.0.0.0:8080");
		std::string web_address("0.0.0.0:8081");

		rev_handler.connect(rev_address);
		std::cout<<"Started reverse listener on "<<rev_address<<std::endl;
		web_handler.connect(web_address);
		std::cout<<"Started web server on "<<web_address<<std::endl;
		while(true)
		{
			rev_handler.update();
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