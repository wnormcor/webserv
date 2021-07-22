#ifndef CONFIG_HPP
# define CONFIG_HPP

#include "includes.hpp"

enum  Method
{
	GET = 0,
	PUT = 1,
	POST = 2,
	HEAD = 3,
	DELETE = 4,
	UNSUPPORTED = 5
};

struct Location
{
	std::string			location;
	std::vector<Method>	methods;
	std::string			index;
	bool				autoindex;
	std::string			root;
	std::string			auth;
	std::string			exec;
	unsigned int		max_body;
};

struct ServerConf
{
	std::string				ip;
	int						port;
	std::string				listen;
	std::string				server_name;
	std::vector<Location>	location;
	std::string				error_page;

	int						ls;
};

class Config {
	int						f_server;		// начало 0; открыто 1; закрыто 0;
	int						f_location;		// начало 0; открыто 1; закрыто 0;
	int						f_error_str;
	std::string	str;
	std::string	conf_str;
	std::string	conf_world;
public:
	int							f_error; 		// 0 без ошибок, 1 ошибки
	ServerConf					_server;
	Location					_location;
	std::vector<ServerConf>		server;
	std::vector<std::string>	conf_tokens;
	std::vector<std::string>	server_tokens;
	std::vector<std::string>	location_tokens;
	//get
	int			getF_server();
	int			getF_location();
	std::string	getStr();
	std::string	getConf_str();
	std::string	getConf_world();
	int			getF_error();
	int			getF_error_str();
	//set
	void	setF_server(int i);
	void	setF_location(int i);
	void	init();
	void	init_server();
	void	init_location();
	void	setF_error_str();
	void	setServer();
	//check
	int		check_file(std::string file);
	int		check_string(std::string str);
	void	check_methods(std::string parameter);
	//other
	void			clear_location();
	unsigned int	str_to_un_int(std::string);
	void			parser_conf(std::string file);
	int				parser_str();
	void			filling_config();
	void			filling_server();
	void			filling_location();
	std::string		getting_parameter(std::string str);
	void			parsing_ip(std::string parametr);
	void			filling_parameter_server(std::string name , std::string parameter);
	void			filling_parameter_location(std::string name , std::string parameter);
	int				validator_end_std(char i);
	void			see_config();
	void			loop(std::string file);
};

#endif