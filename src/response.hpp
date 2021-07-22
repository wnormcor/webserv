#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "includes.hpp"

// прототип Method
#include "config.hpp"

enum ResponseCode {
	Ok = 200,
	Created = 201,
	BadRequest = 400,
	Forbidden = 403,
	NotFound = 404,
	MethodNotAllowed = 405,
	PayloadTooLarge = 413,
	InternalServerError = 500,
	NotImplemented = 501,
	HTTPVersionNotSupported = 505
};

// структура для подготовки ответа на запрос
struct Response
{	
	const std::string					httpVersion;						// версия всегда HTTP/1.1 
	int									status_code_int_val;				// код ответа сервера
	std::string							reason_phrase;						// кодовая фраза ответа
	std::map<std::string, std::string>	headers;							// мапа для хранения заголовков
	std::string 						body;								// тело ответа
	
										Response();							// конструктор (устанавливаем версию протокола)
	Response &							operator=( Response const & rhs );	// присваивание - для возврата в main
	std::string							getStrForAnswer();					// сформировать код ответа (для отдачи клиенту - когда тело заполнены в handlerequest)
};

// прототип для вставки в handlerequest
char *getTime(char *mbstr, const char *format, const char *pagePath, bool lm);

// прототип для вставки в handlerequest
Response ReturnErrorPage(ResponseCode response_code, std::vector<Method> *allowedMethod, Method req_method, ServerConf *server_conf);

#endif