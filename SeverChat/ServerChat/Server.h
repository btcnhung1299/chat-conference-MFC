#pragma once

#include "resource.h"

#include <string>
#include <thread>

#define FILE_BUFFER_SIZE 10240 // The size of buffer for receiving file in bytes
#define UCACHE_LENGTH 125
#define SUBPORT_LENGTH 5
#define CHAT	L"[CHAT] "
#define RESOL	L"[RESOLVER] "
#define HANDLR	L"[HANDLER] "

#define PORT_STT_FREE 1
#define PORT_STT_BUSY 0

#define MESSAGE_NEW_USER	WM_USER + 1
#define MESSAGE_NEW_LOG	WM_USER + 2


struct User {
	bool available;
	CString _address;
	UINT inboxPort;
	std::string nickname;
};


bool checkNickName(std::string nickname, User* users);

void InboxUsers(std::string senderName, std::string message, User * userCache);

void AssignNewUser(CSocket * tmpSockPtr, User * userCache, bool& end);

void connectResolver(User * userCache, std::string& toScreen, int * portsStatus, bool& end, bool& coutBlocked, CDialog& dlg);

void recieveListener(int portNumber, int& portStatus, User * userCache, bool& end, bool& coutBlocked, CDialog& dlg);

void terminateListen(bool * end);