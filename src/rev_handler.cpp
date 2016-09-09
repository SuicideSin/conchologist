#include "rev_handler.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <msl/crypto.hpp>
#include <msl/string.hpp>
#include <pack_util.hpp>
#include <stdexcept>
#include <time.h>

//Really just seconds...
static int64_t millis()
{
	int64_t now=time(0);
	return 1000*now;
}

//Need to recv a key within this time or else it is assumed the connection is plaintext...
static const int64_t KEY_RECV_TIMEOUT_MS=10000;

static const std::string key_head1("-----BEGIN RSA PUBLIC KEY-----\n");
static const std::string key_head2("-----BEGIN PUBLIC KEY-----\n");
static const std::string key_tail1("-----END RSA PUBLIC KEY-----\n");
static const std::string key_tail2("-----END PUBLIC KEY-----\n");

static void ev_handler(mg_connection* conn,int event,void* p)
{
	struct mbuf* io=&conn->recv_mbuf;
	(void)p;
	rev_handler_t& handler(*(rev_handler_t*)(conn->mgr->user_data));
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

rev_handler_t::rev_handler_t(rev_handler_cb_t add_cb,rev_handler_cb_t remove_cb,
	rev_handler_cb_t recv_cb):
	connected_m(false),add_cb_m(add_cb),remove_cb_m(remove_cb),recv_cb_m(recv_cb)
{
	std::string line;
	std::ifstream privf("private_key.pem");
	if(!privf)
		throw std::runtime_error("CC handler could not find \"private_key.pem\".");
	while(std::getline(privf,line))
		private_key_m+=line+"\n";
	privf.close();
}

rev_handler_t::~rev_handler_t()
{
	if(connected_m)
		mg_mgr_free(&mgr_m);
}

void rev_handler_t::connect(const std::string& address)
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

void rev_handler_t::update()
{
	if(connected_m)
	{
		mg_mgr_poll(&mgr_m,1);
		for(rev_client_map_t::iterator it=clients_m.begin();it!=clients_m.end();++it)
		{
			rev_client_t& client=it->second;
			if(client.status==UNKNOWN&&millis()>client.timeout)
			{
				client.status=PLAINTEXT;
				for(size_t ii=0;ii<client.buffered_cmds.size();++ii)
					send(client.address,client.buffered_cmds[ii]);
				client.buffered_cmds.clear();
				if(recv_cb_m)
					recv_cb_m(*this,client);
			}
			it->first->last_io_time=mg_time();
		}
	}
}

void rev_handler_t::add(mg_connection* conn)
{
	rev_client_t client;
	char buffer[200];
	mg_conn_addr_to_str(conn,buffer,200,
		MG_SOCK_STRINGIFY_IP|MG_SOCK_STRINGIFY_PORT|MG_SOCK_STRINGIFY_REMOTE);
	client.alive=true;
	client.status=UNKNOWN;
	client.address=buffer;
	client.timeout=millis()+KEY_RECV_TIMEOUT_MS;
	clients_m[conn]=client;
	if(add_cb_m)
		add_cb_m(*this,client);
}

void rev_handler_t::remove(mg_connection* conn)
{
	clients_m[conn].alive=false;
	if(remove_cb_m)
		remove_cb_m(*this,clients_m[conn]);
	clients_m.erase(conn);
}

//Recv from reverse connection
//  Everything from connection starts with "  " when stored in chunks...
void rev_handler_t::recv(mg_connection* conn,std::string buffer)
{
	rev_client_t& client=clients_m[conn];
	bool status_changed=false;
	if(client.status==UNKNOWN)
	{
		client.scratch+=buffer;
		buffer="";
		if(client.scratch.size()>0)
		{
			size_t min_size1=std::min(client.scratch.size(),key_head1.size());
			size_t min_size2=std::min(client.scratch.size(),key_head2.size());
			if(client.scratch.substr(0,min_size1)!=key_head1.substr(0,min_size1)&&
				client.scratch.substr(0,min_size2)!=key_head2.substr(0,min_size2))
			{
				client.status=PLAINTEXT;
				buffer=client.scratch;
				client.scratch="";
				status_changed=true;
			}
			else if(min_size1>=key_head1.size()||min_size2>=key_head2.size())
			{
				client.status=RECV_PUBKEY;
			}
		}
	}
	if(client.status==PLAINTEXT)
	{
		client.chunks.push_back("  "+buffer);
		if(recv_cb_m)
			recv_cb_m(*this,client);
	}
	if(client.status==RECV_PUBKEY)
	{
		client.scratch+=buffer;
		buffer="";
		for(size_t ii=0;ii<client.scratch.size();++ii)
		{
			buffer+=client.scratch[ii];
			if(msl::ends_with(buffer,key_tail1)||msl::ends_with(buffer,key_tail2))
			{
				std::string key=buffer;
				buffer=client.scratch.substr(key.size(),client.scratch.size()-key.size());
				client.scratch="";
				client.pubkey=key;
				client.status=CHALLENGE;
				break;
			}
		}
	}
	if(client.status==CHALLENGE)
	{
		for(size_t ii=0;ii<buffer.size();++ii)
		{
			client.scratch+=buffer[ii];
			if(buffer[ii]=='\n')
			{
				try
				{
					buffer=buffer.substr(ii+1,buffer.size()-ii-1);
					std::string challenge=pack_hex(msl::decrypt_rsa(unpack_hex(client.scratch),private_key_m));
					client.scratch="";
					mg_send(conn,challenge.c_str(),challenge.size());
					try
					{
						client.secret=msl::crypto_rand(32);
						client.iv=msl::crypto_rand(16);
						std::string packed_secret=pack_rsa(client.secret,client.pubkey);
						std::string packed_iv=pack_rsa(client.iv,client.pubkey);
						mg_send(conn,packed_secret.c_str(),packed_secret.size());
						mg_send(conn,packed_iv.c_str(),packed_iv.size());
						status_changed=true;
						client.status=ENCRYPTED;
					}
					catch(...)
					{
						std::cout<<client.address<<" Error Sending Symmetric Key"<<std::endl;
						kill(client.address);
					}
				}
				catch(...)
				{
					std::cout<<client.address<<" Invalid Client Challenge"<<std::endl;
					kill(client.address);
				}
				break;
			}
		}
	}
	if(client.status==ENCRYPTED&&(!status_changed||buffer.size()>0))
	{
		try
		{
			std::string rm;
			std::string line;
			for(size_t ii=0;ii<buffer.size();++ii)
			{
				line+=buffer[ii];
				if(isspace(buffer[ii]))
				{
					line=unpack_aes(line,client.secret,client.iv);
					client.chunks.push_back("  "+line);
					rm+=line;
					line="";
					if(recv_cb_m)
						recv_cb_m(*this,client);
				}
			}
			buffer=buffer.substr(rm.size(),buffer.size()-rm.size());
		}
		catch(...)
		{
			std::cout<<client.address<<" Bad Decrypt"<<std::endl;
		}
	}
	if(status_changed)
	{
		for(size_t ii=0;ii<client.buffered_cmds.size();++ii)
			send(client.address,client.buffered_cmds[ii]);
		client.buffered_cmds.clear();
	}
}

//Send to reverse connection
//  Everything to connection starts with "$ " when stored in chunks...
void rev_handler_t::send(const std::string& address,std::string buffer)
{
	for(rev_client_map_t::iterator it=clients_m.begin();it!=clients_m.end();++it)
	{
		rev_client_t& client=it->second;
		if(client.address==address)
		{
			if(client.status==PLAINTEXT||client.status==ENCRYPTED)
			{
				client.chunks.push_back("$ "+buffer);
				if(client.status==ENCRYPTED)
				{
					try
					{
						buffer=pack_aes(buffer,client.secret,client.iv);
					}
					catch(...)
					{
						std::cout<<client.address<<" Bad Encrypt"<<std::endl;
					}
				}

				mg_send(it->first,buffer.c_str(),buffer.size());
			}
			else
			{
				client.buffered_cmds.push_back(buffer);
			}
		}
	}
}

void rev_handler_t::kill(const std::string& address)
{
	for(rev_client_map_t::iterator it=clients_m.begin();it!=clients_m.end();++it)
		if(it->second.address==address)
			it->first->flags|=MG_F_CLOSE_IMMEDIATELY;
}

rev_client_list_t rev_handler_t::list() const
{
	rev_client_list_t clients;
	for(rev_client_map_t::const_iterator it=clients_m.begin();it!=clients_m.end();++it)
		clients.push_back(it->second);
	return clients;
}

const rev_client_map_t& rev_handler_t::map() const
{
	return clients_m;
}