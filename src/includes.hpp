#ifndef INCLUDES_HPP
#define INCLUDES_HPP

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
# include <iostream>
#include <list>
#include <vector>
#include <fstream>
#include <cstdlib>
# include <sys/stat.h>
# include <sys/types.h>
#include <map>

// чтобы делать листинг директории
#include <dirent.h>

// for ifstream
#include <fstream>

// for std::stringstream
#include <sstream>

// создание сокета
#include <sys/socket.h>

// структура интернет/универсальных адресов для бинда и акцепта
#include <netinet/in.h>

// конвертация адресов для стуктур addr_in:
// 		inet_addr()
//		hton*() <-> ntoh*()
#include <arpa/inet.h>

// управление дискриптором:
//		fcntl()
#include <fcntl.h>

// для блокировки сигналов
#include <signal.h>

#endif