#ifndef MAIN_HPP
# define MAIN_HPP

#define BUF_SIZE 5000000			// для малых тестов 4096

#include "includes.hpp"
#include "config.hpp"

// парсер запросов
#include "parserequest.hpp"

struct Session
{
	enum			AcceptState {accept_client = 1, read_client = 2, write_client = 3, close_client = 4,
								 change_to_write = 5, change_to_read = 6};

	enum			SessionState {read = 0, write = 1, close = 2};

	ServerConf		*server_conf;	// конфиг сервера для сессии

	int				sd;				// дискриптор сокета соединения
	SessionState	ss;				// состояние сессии читаем/пишем/закрыто
    std::string		client_ip;		// для дебага и CGI
	uint16_t		client_port;	// для дебага

	Request			request_obj;	// объект обработки данных запросов - сюда сохраняем и очищаем
	std::string 	answer_buf;		// буфер ответа
};

// чтобы можно было вызывать обработчик запросов (использует Session)
#include "handlerequest.hpp"

#endif