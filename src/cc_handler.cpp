#include "cc_handler.hpp"

#include <stdexcept>

static void ev_handler(mg_connection* conn,int event,void* p)
{
	struct mbuf* io=&conn->recv_mbuf;
	(void)p;
	cc_handler_t& handler(*(cc_handler_t*)(conn->mgr->user_data));
	switch(event)
	{
		case MG_EV_ACCEPT:
			handler.add(conn);
			break;
		case MG_EV_CLOSE:
			handler.remove(conn);
			break;
		case MG_EV_RECV:
			handler.recv(conn,std::string(io->buf,io->len));
			mbuf_remove(io,io->len);
			break;
		default:
			break;
	}
}

cc_handler_t::cc_handler_t(cc_handler_cb_t add_cb,cc_handler_cb_t remove_cb,
	cc_handler_cb_t recv_cb):
	connected_m(false),add_cb_m(add_cb),remove_cb_m(remove_cb),recv_cb_m(recv_cb)
{}

cc_handler_t::~cc_handler_t()
{
	if(connected_m)
		mg_mgr_free(&mgr_m);
}

void cc_handler_t::connect(const std::string& address)
{
	if(!connected_m)
	{
		mg_mgr_init(&mgr_m,this);
		if(mg_bind(&mgr_m,address.c_str(),ev_handler)==NULL)
		{
			mg_mgr_free(&mgr_m);
			throw std::runtime_error("CC handler could not listen on "+address+".");
		}
		connected_m=true;
	}
}

void cc_handler_t::update()
{
	if(connected_m)
		mg_mgr_poll(&mgr_m,1);
}

void cc_handler_t::add(mg_connection* conn)
{
	cc_client_t client;
	char buffer[200];
	mg_conn_addr_to_str(conn,buffer,200,
		MG_SOCK_STRINGIFY_IP|MG_SOCK_STRINGIFY_PORT|MG_SOCK_STRINGIFY_REMOTE);
	client.address=buffer;
	clients_m[conn]=client;
	if(add_cb_m)
		add_cb_m(*this,client);
}

void cc_handler_t::remove(mg_connection* conn)
{
	if(remove_cb_m)
		remove_cb_m(*this,clients_m[conn]);
	clients_m.erase(conn);
}

void cc_handler_t::recv(mg_connection* conn,const std::string& buffer)
{
	std::string line;
	for(size_t ii=0;ii<buffer.size();++ii)
	{
		if(buffer[ii]!='\n')
			line+=buffer[ii];

		if(buffer[ii]=='\n'||ii+1>=buffer.size())
		{
			clients_m[conn].history.push_back(line);
			line="";
			continue;
		}
	}
	if(recv_cb_m)
		recv_cb_m(*this,clients_m[conn]);
}

void cc_handler_t::send(const std::string& address,const std::string& buffer)
{
	for(cc_client_map_t::iterator ii=clients_m.begin();ii!=clients_m.end();++ii)
		if(ii->second.address==address)
			mg_send(ii->first,buffer.c_str(),buffer.size());
}

cc_client_list_t cc_handler_t::list() const
{
	cc_client_list_t clients;
	for(cc_client_map_t::const_iterator ii=clients_m.begin();ii!=clients_m.end();++ii)
		clients.push_back(ii->second);
	return clients;
}

const cc_client_map_t& cc_handler_t::map() const
{
	return clients_m;
}