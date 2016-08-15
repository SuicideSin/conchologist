#include "web_handler.hpp"

#include <stdexcept>
#include <string.h>

static void ev_handler(mg_connection* conn,int event,void* p)
{
	if(event==MG_EV_CLOSE)
	{
		web_handler_t& handler(*(web_handler_t*)(conn->mgr->user_data));
		handler.close(conn);
	}
	else if(event==MG_EV_HTTP_REQUEST)
	{
		http_message* msg=(http_message*)p;
		if(msg==NULL)
			return;
		char buffer[200];
		mg_conn_addr_to_str(conn,buffer,200,
			MG_SOCK_STRINGIFY_IP|MG_SOCK_STRINGIFY_PORT|MG_SOCK_STRINGIFY_REMOTE);
		web_client_t client;
		client.conn=conn;
		client.address=std::string(buffer);
		client.method=std::string(msg->method.p,msg->method.len);
		client.request=std::string(msg->uri.p,msg->uri.len);
		client.query=std::string(msg->query_string.p,msg->query_string.len);
		client.post_data=std::string(msg->body.p,msg->body.len);
		web_handler_t& handler(*(web_handler_t*)(conn->mgr->user_data));
		bool serviced=handler.service(client);
		if(!serviced)
		{
			if(client.method=="GET")
			{
				mg_serve_http_opts opts;
				memset(&opts,0,sizeof(opts));
				opts.document_root=handler.web_root().c_str();
				opts.enable_directory_listing="no";

				mg_serve_http(conn,msg,opts);
			}
			else if(client.method=="POST")
			{
				handler.send(client,"400 Bad Request","");
			}
			else
			{
				handler.send(client,"405 Method Not Allowed","");
			}
		}
	}
}

web_handler_t::web_handler_t(const std::string& web_root,
	web_handler_cb_t service_cb,web_handler_close_cb_t close_cb):
	connected_m(false),
	web_root_m(web_root),
	service_cb_m(service_cb),
	close_cb_m(close_cb)
{}

web_handler_t::~web_handler_t()
{
	if(connected_m)
		mg_mgr_free(&mgr_m);
}

void web_handler_t::connect(const std::string& address)
{
	if(!connected_m)
	{
		mg_mgr_init(&mgr_m,this);
		mg_connection* server=mg_bind(&mgr_m,address.c_str(),ev_handler);
		if(server==NULL)
		{
			mg_mgr_free(&mgr_m);
			throw std::runtime_error("Web handler could not listen on "+address+".");
		}
		mg_set_protocol_http_websocket(server);
		connected_m=true;
	}
}

void web_handler_t::update()
{
	if(connected_m)
		mg_mgr_poll(&mgr_m,1);
}

std::string web_handler_t::web_root() const
{
	return web_root_m;
}

bool web_handler_t::service(web_client_t& client)
{
	if(service_cb_m)
		return service_cb_m(*this,client);
	return false;
}

void web_handler_t::close(mg_connection* conn)
{
	if(close_cb_m)
		close_cb_m(*this,conn);
}

void web_handler_t::send(const web_client_t& client,const std::string& status,
	const std::string& content)
{
	mg_printf(client.conn,
		"HTTP/1.1 %s\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: %ld\r\n"
		"\r\n"
		"%s",
		status.c_str(),content.size(),content.c_str());
}