#ifndef WEB_HANDLER_HPP
#define WEB_HANDLER_HPP

#include <cstdlib>
#include <mongoose/mongoose.h>
#include <string>

struct web_client_t
{
	mg_connection* conn;
	std::string address;
	std::string client;
	std::string method;
	std::string request;
	std::string query;
	std::string post_data;

};
class web_handler_t;

typedef bool(*web_handler_cb_t)(web_handler_t&,const web_client_t&);

class web_handler_t
{
	public:
		web_handler_t(const std::string& web_root,
			web_handler_cb_t service_cb=NULL);
		~web_handler_t();
		void connect(const std::string& address);
		void update();
		std::string web_root() const;
		bool service(const web_client_t& client);
		void send(const web_client_t& client,const std::string& status,
			const std::string& content);
	private:
		web_handler_t(const web_handler_t& copy);
		web_handler_t& operator=(const web_handler_t& copy);
		bool connected_m;
		mg_mgr mgr_m;
		std::string web_root_m;
		web_handler_cb_t service_cb_m;
};

#endif