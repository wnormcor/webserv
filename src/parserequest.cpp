#include "parserequest.hpp"

// создание вектора строк на основе разделителя
std::vector<std::string>	split(std::string req, std::string delimeter)
{
	std::vector<std::string>	v;
	std::string					word;
	std::size_t					begin = 0;
	std::size_t					end = 0;
	std::size_t					found = 0;

	while ( (found = req.find(delimeter, begin)) != std::string::npos)
	{
		end = found;
		word = req.substr(begin, end - begin);
		if (word != "")
			v.push_back(word);
		begin = found + delimeter.length();
	}
	return (v);
}

// создание объекта и очистка запроса
			Request::Request()
{
	this->is_have_body = false;
	this->content_length = 0;
	this->is_chunked = false;
	this->length_of_TE = 0;
	this->is_request_ok = false;
	this->is_bad_request = false;
}

void		Request::clear()
{
	this->vector_of_lines.clear();
	this->vector_of_starline.clear();
	this->protocol_version = "";
	this->request_target = "";
	this->method_string = "";
	this->unparser_remainder.clear();
	this->_body = "";
	this->is_have_body = false;
	this->map_of_headers.clear();
	this->content_length = 0;
	this->is_chunked = false;
	this->length_of_TE = 0;
	this->temp_body = "";
	this->is_request_ok = false;
	this->method_int = UNSUPPORTED;
	this->query_string = "";
	this->is_bad_request = false;
}

Request		*Request::parse_request(std::string req)
{
	// добавим к запросу остаток (если он есть - вторая и полследующая обработки запроса)
	req = this->unparser_remainder + req;

	// debug - печать запроса
	// std::cout << "\x1b[32m" << req <<  "\x1b[0m" << '\n';

	// если:
	// - первые символы буфера - перенос строки \r\n
	// - длинна контента 0
	// - трансфер энкодинга нет по чанкам 
	// - объем переданного старта 3 позиции
	// => запрос успешный -> отдамем его в мейне в обработку
	if (req[0] == '\r' && req[1] == '\n' && this->content_length == 0 && this->is_chunked == false  && vector_of_starline.size() == 3)	
		this->is_request_ok = true;

	// если запрос закончен - то возвращем ссылку на себя
	if (this->is_request_ok)
		return (this);

	// если есть флаги для содержания - добавляем тело из запроса
	if (this->is_have_body)
		this->temp_body = req;
	else
	{
		// установим тело запроса
		req = this->set_body(req);

		// разделим строку на вектора по разделителю переноса строки
		this->vector_of_lines = split(req, "\r\n");

		// =================== определить остаток (unparser_remainder):
		// в остаток помещаем все что справа от переноса строки
		size_t	found;

		found = req.rfind("\r\n");
		if (found != std::string::npos)
		{
			found += 2;
			this->unparser_remainder = req.substr(found);
		}
		else
			this->unparser_remainder = req;
		// =================== end of set remainder

		if ((this->vector_of_lines).size() >= 1)
		{
			if (this->method_string == "" && this->request_target == "" && this->protocol_version == "")
			{
				// если старлайн еще не задан - парсим старлайн
				this->set_start_line();
				if (this->is_request_ok)
					return (this);
			}
			if ((this->vector_of_lines).size() > 0)
			{
				// выставляем заголовки
				this->set_headers();
				if (this->is_request_ok)
					return (this);
			}
		}
	}
	if (this->is_have_body)
		this->check_body();

	return (this);
}

std::string	Request::set_body(std::string req)
{
	size_t		found;
	std::string	new_req;

	if (req[0] == '\r' && req[1] == '\n' && this->vector_of_starline.size() == 3) // иногда по стандарту может быть перенос строки вначале запроса - учитываем это
	{
		this->temp_body = req.substr(2);
		this->is_have_body = true;
		return ("");
	}
	found = req.find("\r\n\r\n");
	if (std::string::npos != found)
	{
		found += 2;
		new_req = req.substr(0, found);
		found += 2;
		this->temp_body = req.substr(found);
		this->is_have_body = true;
		return (new_req);
	}
	else
		return (req);
}

// определяем стартлайн и парсим ее
void		Request::set_start_line()
{
	std::string					word;
	std::size_t					begin = 0;
	std::size_t					end = 0;
	std::size_t					found = 0;

	// парсим строчку до конца по пробелам
	while (std::string::npos != found)
	{
		found = this->vector_of_lines[0].find(" ", begin);
		end = found;
		word = this->vector_of_lines[0].substr(begin, end - begin);
		if (word == "")
		{
			std::cout << "Error: Start Line" << std::endl;
			this->is_request_ok = true;
			this->is_bad_request = true;
			return ;
		}
		this->vector_of_starline.push_back(word);
		begin = found + 1;
	}

	// если сбой в передаче - возвращаем ошибку (чтобы избежать сбоя из за мусора)
	if (3 != this->vector_of_starline.size())
	{
		std::cout << "Error: Number of arguments in starline" << std::endl;
		this->is_request_ok = true;
		this->is_bad_request = true;
		return ;
	}

	// очищаем вектора по строчкам
	this->vector_of_lines.erase(this->vector_of_lines.begin());

	// метод берем из первой строчки
	this->method_string = this->vector_of_starline[0];
	if (this->method_string == "GET" || this->method_string == "\n\nGET" || this->method_string == "\nGET")
		this->method_int = GET;
	else if (this->method_string == "PUT" || this->method_string == "\n\nPUT" || this->method_string == "\nPUT")
		this->method_int = PUT;
	else if (this->method_string == "HEAD" || this->method_string == "\n\nHEAD" || this->method_string == "\nHEAD")
		this->method_int = HEAD;
	else if (this->method_string == "POST" || this->method_string == "\n\nPOST" || this->method_string == "\nPOST")
		this->method_int = POST;
	else if (this->method_string == "DELETE" || this->method_string == "\n\nDELETE" || this->method_string == "\nDELETE")
		this->method_int = DELETE;
	else
		this->method_int = UNSUPPORTED;

	this->request_target = this->vector_of_starline[1];
	found = this->request_target.find("?");
	if (std::string::npos != found)
		this->query_string = this->request_target.substr(found + 1);
	this->protocol_version = this->vector_of_starline[2];
}

// устанавливаем заголовки по запросу
void	Request::set_headers()
{
	std::string	headLine;
	std::string	key;
	std::string	value;
	size_t		index;

	for (size_t i = 0; i < this->vector_of_lines.size(); i++)
	{
		headLine = this->vector_of_lines[i];
		index = headLine.find(':');
		if (index != std::string::npos)
			key = headLine.substr(0, index);
		else
		{
			this->is_request_ok = true;
			this->is_bad_request = true;
			return ;
		}
		if (key.length() == 0)
		{
			std::cout << "Error Headers (key length)" << std::endl;
			this->is_request_ok = true;
			this->is_bad_request = true;
			return ;
		}
		if (key[0] == ' ' || key[key.length() - 1] == ' ')
		{
			std::cout << "Error Headers (parse_request)" << std::endl;
			this->is_request_ok = true;
			this->is_bad_request = true;
			return ;
		}
		index++;
		if (headLine[index] == ' ')
			index++;
		value = headLine.substr(index);
		if (value.length() == 0)
		{
			std::cout << "Error Headers (value length)" << std::endl;
			this->is_request_ok = true;
			this->is_bad_request = true;
			return ;
		}

		// переводим все заголовки и их содержимое в нижний регистр
		std::transform(key.begin(), key.end(), key.begin(), tolower);
		std::transform(value.begin(), value.end(), value.begin(), tolower);

		// заполняем content-length
		if (key == "content-length")
			this->content_length = std::stoi(value);

		// заполняем флаг chunked (TE)
		if (key == "transfer-encoding" && value == "chunked")
			this->is_chunked = true;

		// заполняем заголовок
		this->map_of_headers.insert(make_pair(key, value));
	}
}

void	Request::check_body()
{
	size_t		found;
	std::string	temp;

	found = 0;
	if (this->is_chunked)
	{
		while (true)
		{
			found = this->temp_body.find("\r\n");
			if (found == 0)
			{
				this->temp_body = this->temp_body.substr(2);
				continue ;
			}
			if (found == std::string::npos)
			{
				this->unparser_remainder = this->temp_body;
				return ;
			}
			if (this->length_of_TE == 0)
			{
				temp = this->temp_body.substr(0, found);
				found += 2;
				this->length_of_TE = std::stoi(temp, NULL, 16);
				if (this->length_of_TE == 0)
				{
					this->is_request_ok = true;
					return ;
				}
				this->temp_body = this->temp_body.substr(found);
				temp = this->temp_body;
			}
			else
				temp = this->temp_body.substr(0, found);
			if (temp.length() > this->length_of_TE)
				temp = this->temp_body.substr(0, this->length_of_TE);

			this->temp_body = this->temp_body.substr(temp.length());
			this->length_of_TE -= temp.length();
			this->_body += temp;
		}
	}
	else if (this->content_length != 0)
	{
		this->_body += this->temp_body;
		if (this->_body.length() >= this->content_length)
		{
			this->_body = this->_body.substr(0, this->content_length);
			this->is_request_ok = true;
		}
	}
	else
		this->is_request_ok = true;
}
