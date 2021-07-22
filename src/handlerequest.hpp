#ifndef HANDLEREQUEST_HPP
# define HANDLEREQUEST_HPP

#include "includes.hpp"

// чтобы видеть структуру Response
#include "response.hpp"

// for delete
#include <cstdio>

// for struct Session
#include "main.hpp"

// for remove
#include <cstdio>

// структура для понимания структуры запрошенного файла (можно перенести в HandleRequest)
struct PathComponents
{
	enum PathType
	{
		ExistingFile = 0,
		ExistingDir = 1,
		NonExistingFile = 2,
		NonExistingDir = 3
	};

	std::string		_path;
	PathType		path_type;

	// опредереляем тип запрошенного ресурса - директория/файл или есть ли или нет
	PathComponents(const std::string &req_target, const std::string &root, size_t location_len);
};

// объявляение прототипа для получения типа по запрошенному файлу
// можно отдавать в класс респонс - для автоматического определения
std::string ReturnContentTypeByExtention(std::string &file_name);

struct HandleRequest
{
	// локация на стеке - заполняется исходя из запроса пользователя и списка локаций в конфиге сервера
	Location	_location;

	// получение типа до файла - обрезание лишних слешей - можно убрать
	std::string get_path_of_page(const Request &req);

	// *****************************************************************************************************************************
	// основной метод обработки запроса - возвращает объект ответа (или вызывает ошибку - которая возвращает тот же объект)
	// при вызове отдаем распарсенный запрос от пользователя и ссылку на конфиг сервера куда пришел запрос (ip:port) + данные сессии
	Response handleRequest(Request &cur_request, ServerConf *server_conf, Session *cur_session);
};

#endif 