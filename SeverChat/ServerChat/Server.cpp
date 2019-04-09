// Project1.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "Server.h"
#include "CommonLib.h"
#include <afxsock.h>
#include <conio.h>
#include <Windows.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only application object

//CWinApp theApp;

using namespace std;


bool checkNickName(string nickname, User* users) {
	for (int i = 0; i < 5; i++)
	{
		if (strcmp(nickname.data(), users[i].nickname.data()) == 0)
		{
			return false;
		}

	}
	return true;
}

void SendLog(CDialog& dlg, CString& message, int type = MESSAGE_NEW_LOG) {
	LPARAM lpr = (LPARAM)(LPCTSTR)message;
	dlg.SendMessage(type, NULL, lpr);
}

void InboxUsers(string senderName, string message, User * userCache) {
	CSocket sender;
	int messageLen;
	bool status = false;
	for (int i = 0; i < UCACHE_LENGTH; i++)
	{
		if (!userCache[i].available)// && (userCache[i].nickname != senderName))
		{
			//Try to connect to every user in user cache.
			sender.Create();
			status = sender.Connect(userCache[i]._address.GetString(), userCache[i].inboxPort);
			if (!(status))
			{
				// User is unavailable -> Free the slot.
				userCache[i].available = true;
				continue;
			}
			//Send nickname to user
			messageLen = senderName.length();
			sender.Send(&messageLen, sizeof(int), 0);
			sender.Send(senderName.data(), messageLen, 0);

			//Send message to user
			messageLen = message.length();
			sender.Send(&messageLen, sizeof(int), 0);
			sender.Send(message.data(), messageLen, 0);

			sender.Close();
		}
	}
}

void AssignNewUser(CDialog& dlg, CSocket * tmpSockPtr, User * userCache, bool& end) {
	CString tmpAddr;
	UINT tmpPort;
	tmpSockPtr->GetPeerName(tmpAddr, tmpPort);
	int freeSlot = -1;
	// Check if user have connected before.
	for (int i = 0; i < UCACHE_LENGTH; i++)
	{
		if (userCache[i]._address == tmpAddr)
		{
			freeSlot = i;
			break;
		}
	}
	if (freeSlot != -1) {
		// Assigned, skip
		tmpSockPtr->Close();
		return;
	}
	// User have never connected.
	//Find a slot
	for (int i = 0; i < UCACHE_LENGTH; i++)
	{
		if (userCache[i].available)
		{
			userCache[i]._address = tmpAddr;
			userCache[i].inboxPort = tmpPort;
			freeSlot = i;
			//cout << HANDLR <<"Stranger" << endl;
			break;
		}
	}
	if (freeSlot == -1)
		//No free slot. Can't assign new user
		return;

	//Set user's nickname to slot

	int strLen = 0;
	UINT inboxPort = 0;
	char buffer[255];
	// Get user's nickname
	tmpSockPtr->Receive(&strLen, 4, 0);
	tmpSockPtr->Receive(&buffer, strLen, 0);
	buffer[strLen] = '\0';
	char ch = ' ';
	if (checkNickName(buffer, userCache))
	{
		userCache[freeSlot].nickname = buffer;
		ch = 'O';
		userCache[freeSlot].available = false;
		tmpSockPtr->Send(&ch, 1, 0);
		// Get user's inbox port
		tmpSockPtr->Receive(&inboxPort, 4, 0);
		userCache[freeSlot].inboxPort = inboxPort;
		//cout << HANDLR << " Assigned " << userCache[freeSlot].nickname << " inbox port : " << userCache[freeSlot].inboxPort << endl;
		string tmpString = "[HANDLER]";
		tmpString += "Assigned";
		tmpString += userCache[freeSlot].nickname;
		CString message(tmpString.c_str(), tmpString.length());
		SendLog(dlg, message);
		message = tmpString.data();
		SendLog(dlg, message,MESSAGE_NEW_USER);
	}
	else
	{
		ch = 'X';
		tmpSockPtr->Send(&ch, 1, 0);
	}
}

void connectResolver(User * userCache, string& toScreen, int * portsStatus, bool& end, bool& coutBlocked, CDialog& dlg) {
	std::this_thread::sleep_for(20ms);
	CSocket mainServer, tempSlot;
	CSocket * tmpSockPtr;
	mainServer.Create(1234);
	CString tmpAddr;
	UINT tmpPort;

	mainServer.GetSockNameEx(tmpAddr, tmpPort);
	CT2CA pszConvertedAnsiString(tmpAddr);
	string ipAddr(pszConvertedAnsiString);
	int stt = 0, errCode = 0;
	CSocket tmpSock;
	//Wait until screen is free
	while (!(dlg.IsWindowVisible())) {

	}
	while (coutBlocked)
	{
	}
	coutBlocked = true;
	CString cString =  RESOL;
	cString += " Started";
	SendLog(dlg, cString);
	coutBlocked = false;
	while (!end)
	{
		mainServer.Listen();
		tmpSockPtr = new CSocket();
		mainServer.Accept(tmpSock);
		CommonData cmData;
		ReceiveCommonData(tmpSock, cmData);
		//cout << cmData.from << cmData.to << cmData.message << cmData.timeStampt.year << cmData.type << cmData.fileSize;
		errCode = mainServer.GetLastError();
		if (errCode > 0)
		{
			//cout << RESOL << " Errcode: " << errCode << endl;
			continue;
		}		
		////cout << RESOL << "Got Request" << endl;
		//tempSlot.GetPeerName(tmpAddr, tmpPort);
		UINT returnPort = 1234;
		for (int i = 0; i < SUBPORT_LENGTH; i++)
		{
			if (portsStatus[i] == PORT_STT_FREE)
			{
				returnPort += i + 1;
				tmpSockPtr->Send(&returnPort, 4, 0);
				break;
			}
		}
		////cout << RESOL << " Respond port " << returnPort << endl;
		tempSlot.Close();
		tmpSock.Close();
		delete tmpSockPtr;
	}
	//cout << RESOL << "Stopped" << endl;
}

void recieveListener(int portNumber, int& portStatus, User * userCache, bool& end, bool& coutBlocked, CDialog& dlg) {
	std::this_thread::sleep_for(100ms);
	CSocket subServer, sock;
	subServer.Create(portNumber);
	int messageLen = 0;
	string message;
	char buffer[255];
	CString clientAddress;
	UINT clientPort;
	int clientIndex = -1;
	//cout << HANDLR << " Started at port " << portNumber << endl;
	while (!(dlg.IsWindowVisible()) || (coutBlocked)) {

	}
	coutBlocked = true;
	CString cString = HANDLR;
	cString += " Started at port: ";
	cString += std::to_string(portNumber).data();
	SendLog(dlg, cString);
	coutBlocked = false;
	int stt = 0;
	char ch = ' ';
	while (!end)
	{
		stt = subServer.Listen();
		subServer.Accept(sock);
		portStatus = PORT_STT_BUSY;
		sock.GetPeerName(clientAddress, clientPort);
		sock.Receive(&ch, 1, 0);
		////cout << HANDLR << " User request: " << ch << endl;
		// User request handling
		// Assigning user to slot
		if (ch == 'a')
		{
			AssignNewUser(dlg, &sock, userCache, end);
		}
		// Chatting
		if (ch == 'c')
		{
			sock.Receive(&messageLen, sizeof(int), 0);
			messageLen = (messageLen > 253) ? (254) : (messageLen);
			sock.Receive(&buffer, messageLen, 0);
			for (int i = 0; i < UCACHE_LENGTH; i++)
			{
				if (userCache[i]._address == clientAddress)
				{
					clientIndex = i;
					break;
				}
			}
			buffer[messageLen] = '\0';
			message = buffer;
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			//cout << CHAT << userCache[clientIndex].nickname << ": " << message << endl;
			InboxUsers(userCache[clientIndex].nickname, message, userCache);
			coutBlocked = false;
		}
		// File sending
		if (ch == 'f')
		{
			//Recieve metadata of the file to transfer
			sock.Receive(&messageLen, sizeof(int), 0);
		}
		sock.Close();
		portStatus = PORT_STT_FREE;
	}
}


void terminateListen(bool * end) {
	char ch = ' ';
	ch = _getch();
	while (ch != '\r')
	{
		ch = _getch();
	}
	//cout << "END" << endl;
	*end = true;    
	return;
}


//int old_main()
//{
//    int nRetCode = 0;
//
//    HMODULE hModule = ::GetModuleHandle(nullptr);
//    if (hModule != nullptr)
//    {
//        // initialize MFC and print and error on failure
//        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
//        {
//            // TODO: change error code to suit your needs
//            wprintf(L"Fatal Error: MFC initialization failed\n");
//            nRetCode = 1;
//        }
//        else
//        {
//            // TODO: code your application's behavior here.
//
//			int a = -1;
//			User * userCache = new User[UCACHE_LENGTH];
//			bool end = false, coutBlocked = false;
//			AfxSocketInit();
//			//cout << "AFXSOCKET INITIALIZED." << endl;
//
//			string messageToScreen;
//
//			CSocket mainServer, mainClient;
//			CSocket subServer[SUBPORT_LENGTH];
//			int * portStatus = new int[SUBPORT_LENGTH] { PORT_STT_FREE };
//
//
//			//thread connector(initConnectResolver, &a, userCache, &end);
//			//Connect Resolver to respond free port
//			thread connector(connectResolver, userCache, ref(messageToScreen), portStatus, ref(end), ref(dlg));
//			//2 sub port to handle user's request
//			thread messager(recieveListener, 1235, ref(portStatus[0]), userCache, ref(end), ref(coutBlocked));
//			thread messager2(recieveListener, 1236, ref(portStatus[1]), userCache, ref(end), ref(coutBlocked));
//			//End this server
//			thread terminator(terminateListen,&end);		
//
//			terminator.join();
//			
//			connector.detach();
//			messager.detach();
//			//messager.join();
//			//cout << " DONE - - - -" << endl;
//			delete[] userCache;
//			getchar();
//			terminate();
//        }
//    }
//    else
//    {
//        // TODO: change error code to suit your needs
//        wprintf(L"Fatal Error: GetModuleHandle failed\n");
//        nRetCode = 1;
//    }
//
//    return nRetCode;
//}
