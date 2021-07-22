#include "config.hpp"

void Config::init() {
	f_server = 0;
	f_location = 0;
	f_error = 0;
	f_error_str = 0;
	conf_tokens.push_back("server");
	server_tokens.push_back("listen");
	server_tokens.push_back("server_name");
	server_tokens.push_back("error_page");
	server_tokens.push_back("location");
	location_tokens.push_back("index");
	location_tokens.push_back("autoindex");
	location_tokens.push_back("methods");
	location_tokens.push_back("auth");
	location_tokens.push_back("exec");
	location_tokens.push_back("root");
	location_tokens.push_back("max_body");
}

void Config::init_server() { 
	_server.listen.clear();
	_server.ip.clear();
	_server.port = 0;
	_server.server_name.clear();
	_server.error_page.clear();
	_server.location = std::vector<Location>();
}

void Config::init_location() {
	Location new_location;
	_location.location.clear();
	_location.index.clear();
	_location.autoindex = false;
	_location.auth.clear();
	_location.exec.clear();
	_location.root.clear();
	_location.max_body = 0;
	_location.methods = std::vector<Method>();
}

void Config::clear_location() {
	_location.index.clear();
	_location.autoindex = false;
	_location.auth.clear();
	_location.exec.clear();
	_location.root.clear();
	_location.max_body = 0;
	_location.methods.clear();
}

int Config::getF_server() {
	return f_server;
}

int Config::getF_location() {
	return f_location;
}

std::string Config::getStr() {
	return str;
}

std::string Config::getConf_str() {
	return conf_str;
}

std::string Config::getConf_world() {
	return conf_world;
}

int Config::getF_error() {
	return f_error;
}

int Config::getF_error_str() {
	return f_error_str;
}

void Config::setF_server(int i) {
	f_server = i;
}

void Config::setF_location(int i) {
	f_location = i;
}

void Config::setF_error_str() {
	f_error_str += 1;
}

int Config::check_string(std::string str) {
	int len = str.length();
	if(str[0] == '/' && str[len - 1] != '/') {
		return 1;
	} else {
		return 0;
	}
}

unsigned int Config::str_to_un_int(std::string str) {
	unsigned int ui = 0;
	int len = str.length();
	for(int i = 0; i < len; i++) {
		if(i != 0) {
			ui *= 10;
		}
		ui += str[i] - '0';
	}
	return ui;
}

int Config::check_file(std::string file) {
	std::ifstream confFile(file);
	if(!confFile) {
		std::cout << "File not find " << file << "!"<< std::endl;
		f_error = 1;
		return 1;
	}
	return 0;
}

void Config::parser_conf(std::string file)
{
	int len;
	if(check_file(file) == 0) {
		std::ifstream confFile(file);
		while(getline(confFile, str)) {
			len = str.length();
			for (int i = 0 ; i < len; i++) {
				if(str[i] != ' ' && str[i] != '\t') 
					conf_str += str[i];
			}
		}
	}
}

int Config::parser_str() {
	static int i = 0;
	conf_world.clear();
	int len = conf_str.length();
	if(i == len) {
		return 0;
	}
	for(; i < len; i++) {
		if(conf_str[i] != '}' && conf_str[i] != '{' && conf_str[i] != ';') {
	 		conf_world += conf_str[i];
		} else if(conf_str[i] == '}' || conf_str[i] == '{' || conf_str[i] == ';') {
			conf_world += conf_str[i];
			i++;
			break ;
		}
	}
	return 1;
}

int Config::validator_end_std(char i) {
	int k = (conf_world.length() - 1);
	if(conf_world[k] == i) {
		return 1;
	} else {
		std::cout << "Entered string [ "<< conf_world << " ] no special character [ " << i << " ] !"<< std::endl;
		f_error = 1; 
	}
	return 0;
}

void Config::parsing_ip(std::string parameter) {
	int len = parameter.length();
	int f_pointer = 0;
	int f_port = 0;
	int lenIp = 0;
	std::string tmp;
	for(int i = 0; i < len; i++) {
		if((parameter[i] >= 48 && parameter[i] <= 58) || (parameter[i] == 46 && f_port == 0)) {
			if(parameter[i] == 46) {
				if(f_pointer > 3 || (lenIp > 3 || lenIp < 1)) {
					std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
					f_error = 1;
				}
				f_pointer++;
				lenIp = -1;
			}
			if(parameter[i] == 58 && f_port == 0) {
				_server.ip = tmp;
				tmp.clear();
				f_port = 1;
				continue;
			}
			lenIp++;
			tmp += parameter[i];
		} else {
			std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
			f_error = 1;
		}
	}
	if(_server.ip.length() == 0) {
		_server.ip = "127.0.0.1";
	}
	if(tmp.length() > 0) {
		_server.port = str_to_un_int(tmp);
	} else {
		std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
		f_error = 1;
	}
}

void Config::filling_parameter_server(std::string name , std::string parameter) {
	if(name == "listen") {
		parsing_ip(parameter);
		_server.listen = parameter;
	} else if(name == "server_name") {
		_server.server_name = parameter;
	} else if(name == "error_page") {
		_server.error_page = parameter; //изменил
		if (check_string(parameter) == 0) {
			f_error = 1;
			std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
		}
	} else if(name == "location") {
		_location.location = parameter;
	}
}

void Config::check_methods(std::string parameter) {
	int len = parameter.length() + 1;
	std::string tmp;
	tmp.clear();
	for(int i = 0; i < len; i++ ) {
		if(tmp == "GET") {
			_location.methods.push_back(GET);
			tmp.clear();
		} else if(tmp == "PUT") {
			_location.methods.push_back(PUT);
			tmp.clear();
		} else if(tmp == "POST") {
			_location.methods.push_back(POST);
			tmp.clear();
		} else if(tmp == "HEAD") {
			_location.methods.push_back(HEAD);
			tmp.clear();
		} else if(tmp == "DELETE") {
			_location.methods.push_back(DELETE);
			tmp.clear();
		} else if(tmp == "UNSUPPORTED") {
			_location.methods.push_back(UNSUPPORTED);
			tmp.clear();
		}
		else if(parameter[i] == ',' || i == len - 1) {
			if(tmp.length() == 0) {
				continue;
			} else {
				std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
				f_error = 1;
			}
		} else {
			tmp += parameter[i];
		}
		
	}

}

void Config::filling_parameter_location(std::string name , std::string parameter) {
	if(name == "methods") {
		_location.methods = std::vector<Method>();
		check_methods(parameter);
	} else if(name == "index") {
		_location.index = parameter;
	} else if(name == "autoindex") {
		if(parameter == "on") {
			_location.autoindex = true;
		} else if(parameter == "off") {
			_location.autoindex = false;
		} else {
			std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
			f_error = 1;
		}
	} else if(name == "auth") {
		_location.auth = parameter;
	} else if(name == "exec") {
		_location.exec = parameter;
	} else if(name == "root") {
		_location.root = parameter; // изменил 
		if (check_string(parameter) == 0) {
			f_error = 1;
			std::cout << "Bad parameter " << getF_error_str() << " [ " << parameter << " ] !" << std::endl;
		}
	} else if(name == "max_body") {
		_location.max_body = str_to_un_int(parameter);
	}
}

std::string Config::getting_parameter(std::string str) {
	std::string parameter;
	int len1 = str.length();
	int len2 = conf_world.length() - 1;
	for(;len1 < len2; len1++) {
		parameter += conf_world[len1];
	}
	return parameter;
}

void Config::filling_config() {
	int f_find = 0;
	char ch = '{';
	init_server();
	for(std::vector<std::string>::iterator i = conf_tokens.begin(); i != conf_tokens.end(); i++) {
		if(conf_world.find(*i) == 0) {
			if(validator_end_std(ch)) {
				f_server = 1;
				f_find = 1;
				break;
			}	
		}
	}
	if(f_find == 0) {
		std::cout << "Bad string " << getF_error_str() << " [ " << conf_world << " ] !" << std::endl;
		f_error = 1;
	}
}

void Config::filling_server() {
	clear_location();
	int f_find = 0;
	char ch = ';';
	if(conf_world == "}") { 
		f_server = 0;
		server.push_back(_server);
	} else {
		for(std::vector<std::string>::iterator i = server_tokens.begin(); i != server_tokens.end(); i++) {
			if(conf_world.find(*i) == 0) {
				if(*i == "location") {
					if(validator_end_std('{')) {
						filling_parameter_server(*i, getting_parameter(*i));
						f_location = 1;
						f_find = 1;
					}
				} else if(validator_end_std(ch)) {
					filling_parameter_server(*i, getting_parameter(*i));
					f_find = 1;
					break;
				}	
			}
		}
		if(f_find == 0) {
			std::cout << "Bad string " << getF_error_str() << " [ " << conf_world << " ] !" << std::endl;
			f_error = 1;
		}
	}
}

void Config::filling_location() {
	int f_find = 0;
	char ch = ';';
	if(conf_world == "}") {
		f_location = 0;
		_server.location.push_back(_location);
	} else {
		for(std::vector<std::string>::iterator i = location_tokens.begin(); i != location_tokens.end(); i++) {
			if(conf_world.find(*i) == 0) {
				if(validator_end_std(ch)) {
					filling_parameter_location(*i, getting_parameter(*i));
					f_find = 1;
				}	
			}
		}
		if(f_find == 0) {
			std::cout << "Bad string " << getF_error_str() << " [ " << conf_world << " ] !" << std::endl;
			f_error = 1;
		}
	}
}

void Config::see_config() {
	std::cout << "size server\t\t" << server.size() << std::endl;
	for(std::vector<ServerConf>::iterator it = server.begin(); it != server.end(); it++) {
		std::cout << "=============== " << "new server\t" << " ===============" << std::endl;
		std::cout << "size location\t\t" << it->location.size() << std::endl;
		std::cout << "listen\t\t\t" << it->listen << std::endl;
		std::cout << "listen ip\t\t" << it->ip << std::endl;
		std::cout << "listen port\t\t" << it->port << std::endl;
		std::cout << "server_name\t\t" << it->server_name << std::endl;
		std::cout << "error_page\t\t" << it->error_page << std::endl;
		for(std::vector<Location>::iterator i = it->location.begin(); i != it->location.end(); i++) {
			std::cout << "=============== " << "new location\t" << " ===============" << std::endl;
			std::cout << "location path\t\t" << i->location << std::endl;
			std::cout << "location index\t\t" << i->index << std::endl;
			std::cout << "location autoindex\t" << i->autoindex << std::endl;
			std::cout << "location auth\t\t" << i->auth << std::endl;
			std::cout << "location exec\t\t" << i->exec << std::endl;
			std::cout << "location root\t\t" << i->root << std::endl;
			std::cout << "location max_body\t" << i->max_body << std::endl;
			std::cout << "size metods\t\t" << i->methods.size() << std::endl;
			for(std::vector<Method>::iterator j = i->methods.begin(); j != i->methods.end(); j++) {
				std::cout << "location methods\t" << *j << std::endl;
			}
		}
	}
}

void Config::loop(std::string file) {
	init();
	init_server();
	init_location();
	parser_conf(file);
	while(parser_str()) {
		setF_error_str();
		if(getConf_world()[0] == '#') {
			continue;
		} else if(getF_server() == 0) {
			filling_config();
		} else if(getF_server() == 1 && getF_location() == 0) {
			filling_server();
		} 
		else if(getF_server() == 1 && getF_location() == 1) {
			filling_location();
		}
	}
	if(getF_error() == 1) {
		std::cout << "Bad config!" << std::endl;
	} else {
		std::cout << "Config accepted!" << std::endl;
	}
	// std::string test = "123456";
	// unsigned int ui = str_to_un_int(test);
	// std::cout << ui << std::endl;
}