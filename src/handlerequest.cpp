#include "handlerequest.hpp"

// req_target - запрошенный ресурс на сервере
// root - путь к локации в конфиге
PathComponents::PathComponents(const std::string &req_target, const std::string &root, size_t location_len)
{
	// возьмем цель без локации (если / есть в локации?)
	// удаляем локацию из запрошеного ресурса
	std::string req_target_with_no_location = req_target;
	req_target_with_no_location.replace(0, location_len, "");
	// удаляем первый символ / из итоговой локации (если есть)
	if ('/' == req_target_with_no_location.front())
		req_target_with_no_location.erase(0, 1);

	// удалим из запрошенного пути последний "/"" если он есть
	std::string root_for_clean = root;
	if ('/' == root_for_clean.back())
		root_for_clean.erase(root_for_clean.size() - 1, 1);

	// в _path поместим полный путь до файла
	_path = root_for_clean + "/" + req_target_with_no_location;

	// определяем тип запрошенного ресурса - файл или директория
	struct stat	path_stat;
	int			result = stat(_path.c_str(), &path_stat);
	if (result != 0)
	{
		if (_path[_path.length() - 1] == '/')
			path_type = NonExistingDir;	
		else
			path_type = NonExistingFile;
	}
	else
	{
		if (S_ISREG(path_stat.st_mode))
			path_type = ExistingFile;
		else
			path_type = ExistingDir;
	}
}

std::string HandleRequest::get_path_of_page(const Request &req)
{
	std::string reqTarget;
	std::string pagePath;

	reqTarget = req.request_target;
	if (reqTarget.length() > 1 && reqTarget[reqTarget.length() - 1] == '/') {
		reqTarget.pop_back();
	}
	if (reqTarget == this->_location.location)
		pagePath = this->_location.root + "/" + this->_location.index;
	else {
		if (this->_location.location == "/")
			pagePath = this->_location.root + reqTarget;
		else
			pagePath = this->_location.root + reqTarget.substr(this->_location.location.length());
	}
	DIR *dir = opendir(pagePath.c_str());
	if (dir) {
		pagePath += "/" + this->_location.index;
		closedir(dir);
	}
	return (pagePath);
}

// основной метод обработки запроса - возвращает объект ответа (или вызывает ошибку - которая возвращает тот же объект)
// при вызове отдаем распарсенный запрос от пользователя и ссылку на конфиг сервера куда пришел запрос (ip:port)
Response HandleRequest::handleRequest(Request &cur_request, ServerConf *server_conf, Session *cur_session)
{

	// если запрос с ошибкой (или не задан хост) - возвращаем сообщение об ошибке
	if (true == cur_request.is_bad_request || 0 == cur_request.map_of_headers.count("host"))
		return (ReturnErrorPage(BadRequest, nullptr, cur_request.method_int, nullptr));

	// ================================================== error in protocol / methods
	// проверка протокола запроса - берем только HTTP/1.1
	if ("HTTP/1.1" != cur_request.protocol_version)
		return (ReturnErrorPage(HTTPVersionNotSupported, nullptr, cur_request.method_int, nullptr));

	// DEBUG
	// std::cout << "RequestMethod: '" << cur_request.method_int << "' is '" << cur_request.method_string << "'" << std::endl;

	// определяем доступность метода для сервера (5 методов)
	if (UNSUPPORTED == cur_request.method_int)
		return (ReturnErrorPage(NotImplemented, nullptr, cur_request.method_int, nullptr));

	// ================================================== choose server_confing
	// определяем хост к которому стучится запрос
	std::string sel_host = cur_request.map_of_headers["host"];
	if (std::string::npos != sel_host.find(":"))
		sel_host = sel_host.substr(0, sel_host.find(":"));

	// определение к какому конфигу обращаемся по имени хоста
	// пока берем по умолчанию текущий конфиг
	// cur_config = _serverCluster._configs[0];
	// for (size_t i = 0; i < _serverCluster._configs.size(); i++)
	// {
	// 	if (host == _serverCluster._configs[i]._server_name)
	// 		cur_config = _serverCluster._configs[i];
	// }
	// определяем конфиг по умолчанию
	ServerConf *sel_server = server_conf;
	(void)sel_server;

	// ================================================== choose location in server config
	// заполняем локацию с которой работаем (локация на стеке)
	_location.location = "";
	bool is_location_finded = false;
	// переберем все локации из конфига
	for (std::vector<Location>::iterator it = sel_server->location.begin(); it < sel_server->location.end(); it++)
	{
		// уберем / в конце пути локации конфига (чтобы можно было корректно сравнивать вхождение)
		std::string location_in_config_to_cmp = it->location;
		if ('/' == location_in_config_to_cmp.back())
			location_in_config_to_cmp = location_in_config_to_cmp.substr(0, location_in_config_to_cmp.size() - 1);

		// если длинна локации в конфиге больше текущей локации и путь в локации совпадает (для /->"" всегда совпадет)
		if (it->location > _location.location && 0 == cur_request.request_target.find(location_in_config_to_cmp))
		{
			_location = *it;
			is_location_finded = true;
		}
	}
	// если локации на сервере не обнаружили - то возвращаем ошибку
	if (false == is_location_finded)
		return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));

	// DEBUG
	// std::cout << "RequestTarget: '" << cur_request.request_target << "' and choosen location: '" << _location.location << "'" << std::endl;
	// std::cout << "PossibleMeths: ";
	// for (std::vector<Method>::iterator it = _location.methods.begin(); it < _location.methods.end(); it++)
		// std::cout << "'" << *it << "'" << ((it != _location.methods.end() - 1) ? ", " : "");
	// std::cout << std::endl;

	// ================================================== check methods in location
	bool is_method_allowed = false;
	for (std::vector<Method>::iterator it = _location.methods.begin(); it < _location.methods.end(); it++)
		if (cur_request.method_int == *it)
			is_method_allowed = true;
	// если метод не разрешен - то возвращаем ошибку и ссылку на ветктор с допустимыми методами
	if (false == is_method_allowed)
		return (ReturnErrorPage(MethodNotAllowed, &_location.methods, cur_request.method_int, server_conf));

	// *** FIND PATH
	// заполнение компонентов пути
	PathComponents path_comp(cur_request.request_target, _location.root, _location.location.length());

	// *** HANDLE REQUEST AND CREATE RESPONSE
	// подготовка ответа - объект ответа на стеке
	Response	response_res;
	// далее важно устанавливать код успешного возврата (или не успешного в случае ошибки)
	// response_res.status_code_int_val = 200;

	// *** AUTOINDEX FUNCTION
	// если запросили директорию методом GET - открываем директорию и подготавливаем ответ в html
	if (PathComponents::ExistingDir  == path_comp.path_type && true == _location.autoindex && GET == cur_request.method_int) {
		
		
		struct dirent *de;													// структура для файла
		std::vector<std::string> dir_file_list;								// вектор списка файлов

		DIR *dir_stream = opendir(path_comp._path.c_str());					// открываем директорию
		while ((de = readdir(dir_stream)) != NULL)
		{
			std::string dir_content_element_name = de->d_name;				// получаем имя файла в директории
			if (DT_DIR == de->d_type)
				dir_content_element_name += "/";							// для директории добавляем '/' в конце
			dir_file_list.push_back(std::string(dir_content_element_name));	// добавляем наименование в вектор
		}
		closedir(dir_stream);												// закрываем директорию

		// устанавливаем успешный код возврата
		response_res.status_code_int_val = 200;

		// составляем тело запроса и отдаем
		response_res.body += "<html>\n<head><title>Autoindex of " + cur_request.request_target + " WebServ</title></head>\n";
		response_res.body += "<body><h1>Autoindex of " + cur_request.request_target + "</h1>\n";
		for (std::vector<std::string>::const_iterator it_begin = dir_file_list.cbegin(); it_begin < dir_file_list.cend(); it_begin++)
			response_res.body += "<a href=\"" + *it_begin + "\">" + *it_begin + "</a>" + "<br>\n";
		response_res.body += "</body>\n</html>\n";

		return response_res;
	}

	// *** GET STATIC FUNCTION
	if ( (GET == cur_request.method_int || HEAD == cur_request.method_int) && (std::string::npos == _location.exec.find(".")) )
	{
		// очистим request_target от queryString
		if (std::string::npos != cur_request.request_target.find("?"))
			cur_request.request_target.erase(cur_request.request_target.find("?"));

		std::string pagePath = this->get_path_of_page(cur_request);

		// // Способ 1: через файловый поток и буфер std::stringstream
		// // медленно - по 800 мс на каждый элемент (по 15-20 кб) ((()))
		// // open ifstream
		// std::ifstream req_file(pagePath.c_str());
		// if(true != req_file.is_open())
		// 	return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));
		// // используем поток stringstream чтобы сохранить файл в буфер
		// std::stringstream buffer;
		// buffer << req_file.rdbuf();
		// // считанный файл в std::string
		// response_res.body = buffer.str();

		// // Способ 2: через буфер и стандартные функции Си
		// // через стандартные функции С - отдает все за 1-1.2 секунды крупные элементы
		// // пробуем открыть файл
		// int fd = open(pagePath.c_str(), O_RDONLY);
		// // если файл не можем открыть - то возввращаем 404 ошибку
		// if (fd == -1)
		// 	return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));
		// // пишем в тело запроса через буфер (для теста что будет быстрее отдавать страницы)
		// int		r;
		// char	buf[4096]; // бефер можно побольше - мб 10 поставить - чтобы разом заглатывать большие файлы
		// while ((r = read(fd, buf, 4096)) > 0)
		// 	response_res.body += std::string(buf, r);
		// if (r < 0)
		// 	return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));	
		// close(fd);

		// Способ 3: через итерации по файловому потоку и посимвольному добавлению в std::string
		std::ifstream req_file(pagePath);
		if(true != req_file.is_open())
			return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));
		response_res.body = std::string((std::istreambuf_iterator<char>(req_file)), std::istreambuf_iterator<char>());
		req_file.close();

		// выставляем код возврата и тип покнтента (в зависимости от расширения файла)
		response_res.status_code_int_val = 200;
		response_res.headers["Content-Type"] = ReturnContentTypeByExtention(pagePath);

		// можно заблокировать для защиты от кеша - чтобы проводить тесты которые выше
		char mbstr[100];
		response_res.headers["Date"] = 			getTime(mbstr, "%a, %d %b %Y %H:%M:%S GMT", pagePath.c_str(), false);
		response_res.headers["Last-Modified"] = getTime(mbstr, "%a, %d %b %Y %H:%M:%S GMT", pagePath.c_str(), true);

		// очищаем тело, если просили только заголовки
		if (HEAD == cur_request.method_int)
			response_res.body = "";

		return response_res;
	}

	// обрабатываем PUT
	if (PUT == cur_request.method_int)
	{
		int fd;
		// int write_result;
		
		if (_location.max_body > 0 && cur_request._body.length() > _location.max_body)
			return (ReturnErrorPage(PayloadTooLarge, nullptr, cur_request.method_int, server_conf));

		if (PathComponents::ExistingDir == path_comp.path_type || PathComponents::NonExistingDir == path_comp.path_type)
			return (ReturnErrorPage(BadRequest, nullptr, cur_request.method_int, server_conf));

		// пытаемся открыть сначала в режиме дозаписи
		fd = open(path_comp._path.c_str(), O_RDWR | O_TRUNC, S_IRWXU);
		if (fd >= 0)
			response_res.status_code_int_val = 200;
		else
		{
			// если неудача - то в режиме создания файла
			fd = open(path_comp._path.c_str(), O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
			if (fd >= 0)
				response_res.status_code_int_val = 201;
			else
				return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		}
		// write_result = write(fd, cur_request._body.c_str(), cur_request._body.length());
		// if (write_result < (long long) cur_request._body.length())
		// 	return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		close(fd);

		// так как файл готов для чтения открываем и дописываем в него данные
		std::ofstream file_for_put(path_comp._path);
		file_for_put << cur_request._body;
		file_for_put.close();

		char mbstr[100];
		response_res.headers["Last-Modified"] = getTime(mbstr, "%a, %d %b %Y %H:%M:%S GMT", path_comp._path.c_str(), true);

		return response_res;
	}

	// обрабатываем post
	if ( (POST == cur_request.method_int) || (GET == cur_request.method_int && std::string::npos != _location.exec.find(".")) )
	{
		if (_location.max_body > 0 && cur_request._body.length() > _location.max_body)
			return (ReturnErrorPage(PayloadTooLarge, nullptr, cur_request.method_int, server_conf));

		// сделаем петлю если запрос идет пустой и максимальной емости 
		if (
			(cur_request._body.length() == _location.max_body || (cur_request._body.length() == 0 && cur_request.query_string.empty()) )
			&& (std::string::npos == _location.exec.find("."))
		)
		{
			response_res.body = cur_request._body;
			response_res.status_code_int_val = 200;
			return response_res;
		}

		// execFile - путь до исполняемого файла
		std::string execFile = std::string(_location.root + "/" + _location.exec);
		// проверяем работоспособность cgi - открываем файл
		int fdCgi = open(execFile.c_str(), O_RDONLY);
		// и смотрим что он есть - если нет - отдаем ошибку - NotFound
		if (_location.exec.empty() || fdCgi == -1)
		{
			close(fdCgi);
			return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));
		}
		close(fdCgi);

		// создадим новый массив для ENV и добавим туда требуемые переменные по стандарту
		std::map<std::string, std::string> map_for_env;
		map_for_env["AUTH_TYPE"]			= _location.auth;
		map_for_env["CONTENT_LENGTH"]		= cur_request.content_length;
		map_for_env["CONTENT_TYPE"]			= cur_request.map_of_headers["content-type"];
		map_for_env["GATEWAY_INTERFACE"]	= "CGI/1.1";
		map_for_env["PATH_INFO"]			= _location.location;
		map_for_env["PATH_TRANSLATED"]		= _location.root + _location.location;
		map_for_env["QUERY_STRING"]			= cur_request.query_string;
		map_for_env["REMOTE_ADDR"]			= cur_session->client_ip;
		map_for_env["REMOTE_IDENT"]			= "Anonymous." + cur_request.map_of_headers["host"];
		map_for_env["REMOTE_USER"]			= "Anonymous";
		map_for_env["REQUEST_METHOD"]		= cur_request.method_string;
		map_for_env["REQUEST_URI"]			= _location.location;
		map_for_env["SCRIPT_NAME"]			= _location.exec;
		map_for_env["SERVER_NAME"]			= server_conf->server_name;
		map_for_env["SERVER_PORT"]			= std::to_string(server_conf->port);
		map_for_env["SERVER_PROTOCOL"]		= cur_request.protocol_version;
		map_for_env["SERVER_SOFTWARE"]		= "newWebServerSchool21";
		// добавляем все заголовки которые были переданы в запросе
		for (std::map<std::string, std::string>::const_iterator itHeader = cur_request.map_of_headers.cbegin();
				itHeader != cur_request.map_of_headers.cend();
				itHeader++)
		{
			map_for_env["HTTP_" + itHeader->first] = itHeader->second;

			// debud addition parser
			// std::cout << "\t\t\t additional headers: " << "HTTP_" << itHeader->first << " and value is " << itHeader->second << std::endl;
		}

		// создадим двухмерный массив переменных окружения для CGI
		char **_env = new char *[map_for_env.size() + 1];	// +1 nullptr
		_env[map_for_env.size()] = nullptr;
		// незабыть очистить двухмерный массив который strdup`пили
		int i = 0;
		for (std::map<std::string, std::string>::const_iterator itENV = map_for_env.cbegin(); itENV != map_for_env.cend(); itENV++, i++)
			_env[i] = strdup((itENV->first + "=" + itENV->second).c_str());

		// список аргументов for execve
		const char *argvForExecutableFile[2] = {_location.exec.c_str(), NULL};

		// откроем файл для записи
		int _input_data = open(std::string(_location.root + "/_input_data").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (_input_data < 0)
			return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		// запишем тело запроса в файл
		// int write_result = write(_input_data, cur_request._body.c_str(), cur_request._body.length());
		// если в файл записали меньше чем было в теле запроса - возвращаем ошибку
		// if (write_result < (long long) cur_request._body.length())
		// 	return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		close(_input_data);

		// запишем тело из поступившего запроса в файл, который потом отдадим на вход cgi-скрипту
		std::ofstream std_input_to_cgi(std::string(_location.root + "/_input_data"));
		std_input_to_cgi << cur_request._body;
		std_input_to_cgi.close();

		// делаем форк
		pid_t id_from_fork = fork();

		// дочерний процесс
		if (0 == id_from_fork)
		{
			// снова откроем файл для чтения чтобы отдать тело
			_input_data = open(std::string(_location.root + "/_input_data").c_str(), O_RDWR);
			if (_input_data < 0)
				exit(EXIT_FAILURE);
			// ввод с терминала перенаправляем на ввод из открытого файла
			dup2(_input_data, 0);

			// файл для отдача ответа
			int _output_data = open(std::string(_location.root + "/_output_data").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if (_output_data < 0)
				exit(EXIT_FAILURE);
			// ввод на терминал перенаправляем на ввывод в открытый файл
			dup2(_output_data, 1);

			// меняем директорию выполнения на место нахождения скрипта
			chdir(_location.root.c_str());
			if (execve(execFile.c_str(), (char * const *) argvForExecutableFile, _env) == -1)
				exit(EXIT_FAILURE);
		}

		// родительский процесс
		// дожидаемся пока ребенок отработает
		int status;
		waitpid(id_from_fork, &status, 0);

		// удаляем файл входящего тела
		remove(std::string(_location.root + "/_input_data").c_str());

		// очищаем _evn
		for (size_t i = 0; _env[i]; i++)
			delete _env[i];
		delete[] _env;

		// прочитаем файл в res_respone.body
		std::ifstream res_cgi_file(_location.root + "/_output_data");
		if (true != res_cgi_file.is_open())
			return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		response_res.body = std::string((std::istreambuf_iterator<char>(res_cgi_file)), std::istreambuf_iterator<char>());
		res_cgi_file.close();

		// удаляем файл отданного тела
		remove(std::string(_location.root + "/_output_data").c_str());

		std::string longsplitter = "\r\n\r\n";
		std::string spliter = "\r\n";

		// первоначально деновский скрипт давал только \n перенос - поправил скрипт - поэтому вариант заблокирован
		// if (std::string::npos != _location.exec.find("."))
		// {
		// 	longsplitter = "\n\n";
		// 	spliter = "\n";
		// }

		// парсинг того что отдал скрипт (отделяем заголовки от тела)
		if (std::string::npos == response_res.body.find(longsplitter))
			return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));
		size_t end_ofheaders_position = response_res.body.find(longsplitter);
		// получим пачку загловкой для обработки
		std::string headers_from_cgi = response_res.body.substr(0, end_ofheaders_position + spliter.length());
		// вырежем заголовки из полей 
		response_res.body.erase(0, end_ofheaders_position + longsplitter.length());

		// вытащим все заголовки и запишем их в структуру ответа - response_res.headers
		size_t find_header_position = 0;
		size_t find_value = 0;
		while (std::string::npos != (find_header_position = headers_from_cgi.find(spliter)))
		{
			std::string header_and_value_from_cgi = headers_from_cgi.substr(0, find_header_position);
			if (std::string::npos != (find_value = header_and_value_from_cgi.find(": "))) {
				std::string key_from_cgi = header_and_value_from_cgi.substr(0, find_value);
				std::string value_from_cgi = header_and_value_from_cgi.substr(find_value + spliter.length());
				response_res.headers[key_from_cgi] = value_from_cgi;
			}
			headers_from_cgi.erase(0, find_header_position + spliter.length());
		}

		// установим статус по умолчанияю
		response_res.status_code_int_val = 200;

		// конвертим в кон статуса цифровые значения из стринга (если скрипт их отдал - python не отдает)
		if (0 != response_res.headers.count("Status"))
			response_res.status_code_int_val = stoi(response_res.headers["Status"]);

		return response_res;
	}

	// обрабатываем delete
	if (DELETE == cur_request.method_int)
	{
		// если запрошена директория - то выдаем ошибку плохого запроса
		if (PathComponents::ExistingDir == path_comp.path_type || PathComponents::NonExistingDir == path_comp.path_type)
			return (ReturnErrorPage(BadRequest, nullptr, cur_request.method_int, server_conf));

		// если файл не найден - то ошибка 404
		if (PathComponents::NonExistingFile == path_comp.path_type)
			return (ReturnErrorPage(NotFound, nullptr, cur_request.method_int, server_conf));

		// удаляем файл - если ошибка даем внутренню ошибку сервера
		if (0 != remove(path_comp._path.c_str()))
			return (ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf));

		// отдаем успешный возврат
		response_res.status_code_int_val = 200;

		return response_res;
	}

	// если другой метод - отдаем ошибку внетренней ошибки
	response_res = ReturnErrorPage(InternalServerError, nullptr, cur_request.method_int, server_conf);

	// отдаем тело ответа
	return (response_res);
}