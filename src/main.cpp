#include "main.hpp"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// вывод текущей сессии - кто подключился/читаем/отдаем/закрываем сессию:
void			debug_connect(Session const &cur_sess, Session::AcceptState client_state, ssize_t count)
{
	if (client_state == Session::accept_client)
		std::cout << "\033[;42m[ Connt for ";
	else if (client_state == Session::read_client)
		std::cout << "\033[;46m[ Reads for "; 
	else if (client_state == Session::change_to_write)
		std::cout << "\033[;43m[ ChgWR for "; 
	else if (client_state == Session::write_client)
		std::cout << "\033[;43m[ Write for "; 
	else if (client_state == Session::change_to_read)
		std::cout << "\033[;46m[ ChgRD for "; 

	else if (client_state == Session::close_client)
		std::cout << "\033[;41m[ Close for ";

		std::cout << (*cur_sess.server_conf).ip << ":"
				  << (*cur_sess.server_conf).port
				  << " from "
				  << cur_sess.client_ip << ":"
				  << cur_sess.client_port;

		std::cout << " (sd:" << cur_sess.sd << ")";

	if (client_state == Session::read_client)
		std::cout << " (" << count << " bytes reads)";
	else if (client_state == Session::write_client)
		std::cout << " (" << count << " bytes write)";

		std::cout << " ]\033[;0m" << std::endl;
}

// фатальная ошибка при инициализации
void		fatal_error(const char *error_str)
{
	perror(error_str);
	exit(EXIT_FAILURE);
}

// перегрузка фатальной ошибки когда не можем поднять сервер -> убрать в utils
void		fatal_error(const char *error_str, const char *ip, const unsigned int port)
{
	std::cout << "For: " << ip << ":" << port << ": " << std::endl;
	perror(error_str);
	exit(EXIT_FAILURE);
}

// вывод текущей конфигурации ServConf -> убрать в utils
std::ostream	&operator<<( std::ostream &o, ServerConf const &i )
{
	o << "[ServerInstance '" << i.server_name << "' (" << i.ip << ":" << i.port << "). LS: " << i.ls << "]"; 
	return o;
}

void		ServerInstanceInit(std::vector<ServerConf> &arraySC)
{
	std::vector<ServerConf>::iterator it;
	for (it = arraySC.begin(); it != arraySC.end(); it++)
	{
		int			sock;

		// 0. создаем слушающий сокет
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1)
			fatal_error("socket");

		// 1. конфиг: чтобы убрать зависание при привязки сокета к порту
		int			opt = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		// 2. присваиваем сокету локальный адрес: привязывает к сокету ls локальный адрес addr длиной addrlen.
		sockaddr_in	addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(it->ip.c_str()); // htonl(INADDR_ANY); -> любой доступный адрес
		addr.sin_port = htons(it->port);
		if(-1 == bind(sock, (struct sockaddr*) &addr, sizeof(addr)))
			fatal_error("bind", it->ip.c_str(), it->port);

		// 3. выражаем готовность принимать входящие соединения и задаем очередь backlog
    	listen(sock, SOMAXCONN);

		it->ls = sock;
	}
}

void		ServerGo(std::vector<ServerConf> &arraySC)
{
	// наборы дискрипторов для проверки готовности
    fd_set	readfds;
    fd_set	writefds;

	// итератор чтобы бегать по поднятым слушающим дискрипторам
	std::vector<ServerConf>::iterator	itSC;
	int									maxfd = -1;

	// хранилице всех клиенских сессий (sd -> session)
	Session								cur_sess;
	std::list<Session>					listSess;
	std::list<Session>::iterator		itListSess;

	// вслучае SIGPIPE - будет игнорировать  - чтобы не получить ошибку выхода при попытке записи в закрытый сокет
	signal(SIGPIPE, SIG_IGN);

    while(true)
	{
        FD_ZERO(&readfds);
		FD_ZERO(&writefds);
		maxfd = 0;

		// ============================= CLEAR CLOSE SESSIONS ============================
		// закроем и почиситим все завершенные сессии
		for (itListSess = listSess.begin(); itListSess != listSess.end();)
			if (Session::close == itListSess->ss)
			{
				close(itListSess->sd);
				itListSess = listSess.erase(itListSess);
			}
			else
				itListSess++;
		// ========================== END CLEAR CLOSE SESSIONS ===========================

		// ============================== PREPARE FOR SELECT =============================
		// занесем в дискрипторы для чтения все слушающие сокеты серверов
		for (itSC = arraySC.begin(); itSC != arraySC.end(); itSC++)
			FD_SET(itSC->ls, &readfds);

		// проверка на случай если ниодного конфига не поднято - переписать
		if (arraySC.size() != 0)
	        maxfd = arraySC.back().ls;

		// обрабатываем массив сессий - и добавляем дискрипторы в набор
		for (itListSess = listSess.begin(); itListSess != listSess.end(); itListSess++)
		{
			if (itListSess->ss == Session::read)
				FD_SET(itListSess->sd, &readfds);
			else if (itListSess->ss == Session::write)
				FD_SET(itListSess->sd, &writefds);
			if (itListSess->sd > maxfd)
				maxfd = itListSess->sd;
		}
		// ============================ END PREPARE FOR SELECT ===========================

		// =================================== SELECT ====================================
        if (-1 == select(maxfd+1, &readfds, &writefds, NULL, NULL))
			fatal_error("select");
		// ================================= END SELECT ==================================

		// =================================== ACCEPT ====================================
		// смотрим все запросы на соединение - если есть - делаем акцепт нового соеденения
		for (itSC = arraySC.begin(); itSC != arraySC.end(); itSC++)
			if (FD_ISSET(itSC->ls, &readfds))
			{

				int			sd;
				sockaddr_in	addr;
				socklen_t	len = sizeof(addr);

				sd = accept(itSC->ls, (struct sockaddr*) &addr, &len);

				// если принять соединение не удалось - выводим ошибку и смотрим следующие сервера
				if (sd == -1) {
					perror("accept");
					continue;
				}

				// устанавливаем неблокирующий режим работы сокета для неблокирующей записи
				fcntl(sd, F_SETFL, O_NONBLOCK);

				// ***********************************************************************************
				// заполняем струтуру сессии на стеке (с которой сейчас работаем) и добавляем в массив

				cur_sess.server_conf = &(*itSC);				// ссылка на конфигрурацию сервера, который принял запрос

				cur_sess.sd = sd;								// созраняем дискриптор соединения с клиентом
				cur_sess.ss = Session::read;					// тип сессии всегда рид вначале (ждем запроса)
				cur_sess.client_ip = inet_ntoa(addr.sin_addr);	// добавим инфу по адресу и порту клиента в сессию (из возвращенной accept() "sockaddr_in addr")
				cur_sess.client_port = ntohs(addr.sin_port);

				cur_sess.request_obj.clear();					// заполнение сессии (ее очистка)
				cur_sess.answer_buf = "";						// очищаем буфер ответа
				
				// добавляем новую сессию в массив
				listSess.push_back(cur_sess);
				// ***********************************************************************************

				// DEBUG - выводим статус в консоль
				debug_connect(cur_sess, Session::accept_client, 0);
			}
		// ============================== END ACCEPT =====================================

		// ========================= READ/WRITE TO CLIENT ================================
		// смотрим все запросы на соединение - если есть - делаем акцепт нового соеденения
		// проходим по всем сессия (в том числе по добавленным и смотрим на предмет возвращения их от селекта)
		for (itListSess = listSess.begin(); itListSess != listSess.end(); itListSess++)
		{

			// сессия в состоянии чтения и дискриптор готов передать данные
			if ((itListSess->ss == Session::read) && (FD_ISSET(itListSess->sd, &readfds)))
			{
				// создаем буфер и читаем из него
				char buf[BUF_SIZE];
				ssize_t read_res = read(itListSess->sd, buf, BUF_SIZE);

				// сконтетинируем в request сессии полученную строку если что-то вернулось
				if (read_res > 0) 
				{	
					// строка для сохранения всего запроса
					// конструктор строки принимает ее длину поэтому можем через буфер получать и \0
					// itListSess->request += std::string(buf, read_res); // убираем копирование в буфев стркои
					itListSess->request_obj.parse_request(std::string(buf, read_res));

					// DEBUG - выводим статус в консоль
					debug_connect(cur_sess, Session::read_client, read_res);

					// проверяем - полный ли запрос?
					if (true == itListSess->request_obj.is_request_ok)
					{
						// закоментил чтобы сразу инициировать объект и передавать туда
						// itListSess->request = "";

						// создание обработчика запросов
						HandleRequest request_handler;

						// ответ по итогам обработки (передаем обработанный запрос/ссылку на конфиг сервера/текущую сессию)
						Response response = request_handler.handleRequest(itListSess->request_obj, itListSess->server_conf, &(*itListSess));

						// сохраняем строку в буфер ответа и переводим сессию в режим записи
						itListSess->answer_buf = response.getStrForAnswer();

						// debug
						// std::cout << ANSI_COLOR_RED <<  std::endl << itListSess->answer_buf << ANSI_COLOR_RESET << std::endl << std::endl;

						// отдаем клиенту контент
						itListSess->ss = Session::write;

						// DEBUG - выводим статус в консоль (меняем на режим чтения)
						debug_connect(cur_sess, Session::change_to_write, 0);
					}
				}
				// если 0 возврат - закрываем сессию
				else
				{ 
					itListSess->ss = Session::close;

					// DEBUG - выводим статус в консоль
					debug_connect(cur_sess, Session::close_client, 0);
				}
			}
			// сессия в состоянии записи и дискрпитор готов принять данные
			else if ((itListSess->ss == Session::write) && (FD_ISSET(itListSess->sd, &writefds)))
			{
				ssize_t write_res = write(itListSess->sd,
										  (itListSess->answer_buf).c_str(),
										  (itListSess->answer_buf).size());
				
				// debug
				// искуственно занижаем объем передачи до 1 байта - проверка корреткности write
				// ssize_t write_res = write(itListSess->sd, (itListSess->answer_buf).c_str(), 1);

				if (write_res > 0)
				{
					// убираем из буфера сессии ту часть, которую передали
					itListSess->answer_buf = (itListSess->answer_buf).substr(write_res, std::string::npos);

					// DEBUG - выводим статус в консоль
					debug_connect(cur_sess, Session::write_client, write_res);
				}
				else
				{
					itListSess->ss = Session::close;

					// DEBUG - выводим статус в консоль
					debug_connect(cur_sess, Session::close_client, 0);
				}

				// все передали? переключаемся в режим чтения или закрытия
				if (0 == (itListSess->answer_buf).size())
				{
					// если требование закрыть соедение - то закрываем соедениение
					if (0 != itListSess->request_obj.map_of_headers.count("connection") &&
						"close" == itListSess->request_obj.map_of_headers["connection"])
					{
						itListSess->ss = Session::close;
						// DEBUG - выводим статус в консоль
						debug_connect(cur_sess, Session::close_client, 0);
					}
					// в противном случае режим чтения и сброс сессии 
					else
					{
						itListSess->ss = Session::read;
						// DEBUG - выводим статус в консоль
						debug_connect(cur_sess, Session::change_to_read, 0);

						// очистка текущей сессии
						itListSess->request_obj.clear();
					}
				}
			}
		}
		// ======================= END READ/WRITE TO CLIENT ==============================
    }
}

int main(int ac, char **av)
{
	Config config;
	
	// определяем файл конфига
	std::string file;
	if(ac == 2)
		file = av[1];
	else
		file = "content/serv.conf";
	config.loop(file);
	// config.see_config();

	// если ошибка - выходим
	if(0 != config.f_error)
		return (EXIT_FAILURE);

	// 0. загрузим конфиг
	std::vector<ServerConf> ServersSets = config.server;

	// 1. инициализируем сетевую часть (создаем сокеты/устанавливаем на порт/слушаем)
	ServerInstanceInit(ServersSets);

	// 2. сервера после инициализации
	std::vector<ServerConf>::iterator it;
	for (it = ServersSets.begin(); it != ServersSets.end(); it++)
		std::cout << "Config server: " << *it << std::endl;

	// 3. стартуем сервера
	ServerGo(ServersSets);

	return (EXIT_SUCCESS);
}