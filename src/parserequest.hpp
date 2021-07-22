#ifndef REQUEST_HPP
# define REQUEST_HPP

#include "includes.hpp"

// для Method
#include "config.hpp"

struct Request
{
	// INTERNAL:
	std::vector<std::string>			vector_of_lines;			// вектор всех строк запроса
	std::vector<std::string>			vector_of_starline;			// вектор первой строки параметров запроса
	std::string							unparser_remainder;			// недораспарсенный остаток

	// STARLINE:
	std::string							method_string;				// запрошенный метод в строке
	Method								method_int;					// запрошенный метод из справочника
	std::string							request_target;				// запрошенный ресурс
	std::string							protocol_version;			// версия протокола
	std::string							query_string;				// строка передачи параметров через GET (если есть)

	// HEADERS:
	std::map<std::string, std::string>	map_of_headers;				// мапа распарсеных заголовков
	size_t								content_length;				// длинна контента
	bool								is_chunked;					// будет ли трансфер-инкодинг - chunked
	size_t								length_of_TE;				// длинна трансфер-инкодинга

	// BODY:
	bool								is_have_body;				// предусмотрено ли содержимое в запросе
	std::string							temp_body;					// буфер для тела запроса
	std::string							_body;						// само тело запроса (если есть)

	// MAIN_FLAGS:
	bool								is_request_ok;				// успешность запроса
	bool								is_bad_request;				// плохой запрос (чтобы отдать ошибку)

										Request();
	Request								*parse_request(std::string req);

	std::string							set_body(std::string req);
	void								set_start_line();
	void								set_headers();
	void								check_body();

	void								clear();
};

#endif