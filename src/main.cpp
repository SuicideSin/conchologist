#include "cc_handler.hpp"
#include <iostream>
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
		handler.send(client,"200 OK","You sent: "+client.post_data);
		return true;
	}
	return false;
}

int main(void)
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