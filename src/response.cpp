#include "response.hpp"

// получение данных о времени
char *getTime(char *mbstr, const char *format, const char *pagePath, bool lm) {
    struct stat		buf;
    int				result;
    std::time_t		t = std::time(NULL);

	// если флаг не нулевой - то запрашиваем время из статы файла и приводим к требуемому виду
    if (lm) {
        result = stat(pagePath, &buf);
        if (result == 0 && std::strftime(mbstr, sizeof(char) * 100, format, std::gmtime(&buf.st_ctimespec.tv_sec)))
            return (mbstr);
    }
	// если нет - то выдаем текущее время
	else
	{
        if (std::strftime(mbstr, sizeof(char) * 100, format, std::gmtime(&t)))
            return (mbstr);
    }
	// в противном случае (какая-то ошибка) просто возвращем пустую строчку
    memset(mbstr, '\0', strlen(mbstr));
    return (mbstr);
}

// функция возврата кодовой фразы по номеру кода rfc-7231
std::string ReturnReasonPhrase(int status_code)
{
	std::string reason_phrase;

	// определяем REASON-PHRASE на основе CODE
	// https://efim360.ru/rfc-7231-protokol-peredachi-giperteksta-http-1-1-semantika-i-kontent/#6-5-Client-Error-4xx

	if (Ok == status_code)
		reason_phrase = "OK";
	else if (Created == status_code)
		reason_phrase = "CREATED";
	else if (BadRequest == status_code)
		reason_phrase = "BAD REQUEST";
	else if (Forbidden == status_code)
		reason_phrase = "FORBIDDEN";
	else if (NotFound == status_code)
		reason_phrase = "NOT FOUND";
	else if (MethodNotAllowed == status_code)
		reason_phrase = "METHOD NOT ALLOWED";
	else if (PayloadTooLarge == status_code)
		reason_phrase = "PAYLOAD TOO LARGE";
	else if (InternalServerError == status_code)
		reason_phrase = "INTERNAL SERVER ERROR";
	else if (NotImplemented == status_code)
		reason_phrase = "NOT IMPLEMENTED";
	else if (HTTPVersionNotSupported == status_code)
		reason_phrase = "HTTP VERSION NOT SUPPORTED";
	else
		reason_phrase = "SOME ERROR";

	return (reason_phrase);
}

// возврат Content-Type по расширению файла
std::string ReturnContentTypeByExtention(std::string &file_name)
{
	std::string extention = file_name.substr(file_name.find_last_of(".") + 1);

	if ("html" == extention)
		return "text/html";
	if ("jpg" == extention)
		return "image/jpeg";
	if ("png" == extention)
		return "image/png";
	if ("css" == extention)
		return "text/css";
	if ("js" == extention)
		return "application/javascript";
	if ("ico" == extention)
		return "image/x-icon";
	if ("gif" == extention)
		return "image/gif";
	if ("ttf" == extention)
		return "font/ttf";

	return "text/plain";
}

// возвращает сформированный объект стандартный страницы ошибки
// отдаем указатель на запрошенный метод - если он будет и это head - то будем убирать боди
Response ReturnErrorPage(ResponseCode response_code, std::vector<Method> *allowedMethod, Method req_method, ServerConf *server_conf)
{
	Response	response;
	std::string	allowed_meths;

	(void)server_conf;
	
	// присвоен код возврата переданному ResponseCode
	response.status_code_int_val = response_code;
	response.headers["Content-Type"] = "text/html";

	// если нужно сформирвоать ошибку MethodNotAllowed - перечисляем доступные методы
	if (MethodNotAllowed == response_code && nullptr != allowedMethod)
	{
		// прохоходим по всем методам и добавляем их в строку
		for (std::vector<Method>::iterator it = allowedMethod->begin(); it < allowedMethod->end(); it++)
		{
			if (GET == *it)
				allowed_meths += "GET";
			else if (HEAD == *it)
				allowed_meths += "HEAD";
			else if (PUT == *it)
				allowed_meths += "PUT";
			else if (POST == *it)
				allowed_meths += "POST";
			else if (DELETE == *it)
				allowed_meths += "DELETE";
			else
				allowed_meths += "UNSUPPORTED";

			// добавляем разделитель если методов несколько
			if (it != allowedMethod->end() - 1)
				allowed_meths += ", ";
		}

		// формируем еще один заголовок
		response.headers["Allow"] = allowed_meths;
	}

	// сформируем код и фразу возврата
	std::string response_code_char = std::to_string(response_code);
	std::string response_phrase = ReturnReasonPhrase(response_code);

	// по умолчанию отдаем страницы по дефолту (или когда не можем определить к какому серверу шел коннект)
	response.body = "<html><head><title>" + response_code_char + " " + response_phrase + "</title></head>" +
					"<body><h1>" + response_code_char + " " + response_phrase + "</h1></body></html>";	

	// Возможность показа страницы сервера из настроек сервера
	if (nullptr != server_conf && false == server_conf->error_page.empty())
	{
		std::ifstream personal_error_file(server_conf->error_page + "/" + std::to_string(response_code) + ".html");
		// если файл открылся - считаме его содержимое в боди
		if (true == personal_error_file.is_open())
			response.body = std::string((std::istreambuf_iterator<char>(personal_error_file)), std::istreambuf_iterator<char>());
		personal_error_file.close();
	}

	// если запросили через HEAD - то убираем тело ошибки
	if (HEAD == req_method)
		response.body = "";

	return (response);
}

// конструктор
Response::Response() : httpVersion("HTTP/1.1"), status_code_int_val(1) {}

Response &		Response::operator=( Response const & rhs )
{
	status_code_int_val = rhs.status_code_int_val;
	reason_phrase = rhs.reason_phrase;
	headers = rhs.headers;
	body = rhs.body;
	return *this;
}

// МЕТОД - сформировать код ответа
std::string Response::getStrForAnswer()
{
	// заполним фразу ответа
	reason_phrase = ReturnReasonPhrase(status_code_int_val);

	// итоговая строка ответа
	std::string		res_str;	

	// формируем старлайн:
	res_str = httpVersion + " " + std::to_string(status_code_int_val) + " " + reason_phrase + "\r\n";

	// добавляем длинну контента если есть контент (всегда отдаем контент-лен)
	if (0 == headers.count("Content-Length"))
		headers["Content-Length"] = std::to_string(body.size());

	// если есть контент и тип не указан добавляем тип по умолчанию - text/html
	if (0 != body.size() && 0 == headers.count("Content-Type"))
		headers["Content-Type"] = "text/html";

	// добавить дату если дата не указана
	char mbstr[100];
	if (0 == headers.count("Date"))
		headers["Date"] = getTime(mbstr, "%a, %d %b %Y %H:%M:%S GMT", "", false);

	// перечисляем заголовки
	for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); it++)
		res_str += it->first + ": " + it->second + "\r\n";

	res_str += "\r\n";
	res_str += body;

	// возвращаем тело ответа
	return res_str;
}