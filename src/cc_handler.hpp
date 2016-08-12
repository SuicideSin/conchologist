#ifndef CC_HANDLER_HPP
#define CC_HANDLER_HPP

#include <cstdlib>
#include <map>
#include <mongoose/mongoose.h>
#include <string>
#include <vector>

struct cc_client_t
{
	std::string address;
	std::vector<std::string> history;
};

class cc_handler_t;

typedef std::map<mg_connection*,cc_client_t> cc_client_map_t;
typedef void(*cc_handler_cb_t)(cc_handler_t&,const cc_client_t&);

class cc_handler_t
{
	public:
		cc_handler_t(cc_handler_cb_t add_cb=NULL,cc_handler_cb_t remove_cb=NULL,
			cc_handler_cb_t recv_cb=NULL);
		~cc_handler_t();
		void connect(const std::string& address);
		void update();
		void add(mg_connection* conn);
		void remove(mg_connection* conn);
		void recv(mg_connection* conn,const std::string& buffer);
		void send(const std::string& address,const std::string& buffer);
		std::vector<cc_client_t> list() const;
	private:
		cc_handler_t(const cc_handler_t& copy);
		cc_handler_t& operator=(const cc_handler_t& copy);
		bool connected_m;
		mg_mgr mgr_m;
		cc_client_map_t clients_m;
		cc_handler_cb_t add_cb_m;
		cc_handler_cb_t remove_cb_m;
		cc_handler_cb_t recv_cb_m;
};

#endif