// Project1.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "Server.h"
#include "CommonLib.h"
#include <afxsock.h>
#include <conio.h>
#include <Windows.h>
#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define LOGIN_SUC 0
#define LOGIN_PASS_WRONG 1
#define LOGIN_NOT_EXIST 2

// The one and only application object

//CWinApp theApp;

using namespace std;

void Wait4Free(bool& isBlocked) {
	while (isBlocked)
	{

	}
	isBlocked = true;
}

void Wait4Free(DlgLogger& logger) {
	while (!(logger.dlg->IsWindowVisible()) || logger.isBlocked)
	{

	}
	logger.isBlocked = true;
}

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

void SendLog(DlgLogger& logger, CString& message, int type = MESSAGE_NEW_LOG) {
	Wait4Free(logger);
	LPARAM lpr = (LPARAM)(LPCTSTR)message;
	logger.dlg->SendMessage(type, NULL, lpr);
	logger.isBlocked = false;
}

void InboxUsers(DlgLogger& logger, string senderName, CommonData &data, User * userCache) {
	CSocket sender;
	bool status = false;
	string toName = data.to;


	CString logLine;
	logLine += CHATP;
	logLine += data.from.c_str();
	logLine += L" -> ";
	logLine += data.to.c_str();
	logLine += L" : ";
	logLine += data.message.c_str();

	for (int i = 0; i < UCACHE_LENGTH; i++)
	{
		if (!userCache[i].available && userCache[i].nickname == toName)
		{
			//Try to connect to every user in user cache.
			sender.Create();
			status = sender.Connect(userCache[i]._address.GetString(), userCache[i].inboxPort);
			if (!(status))
			{
				// User is unavailable -> Free the slot.
				disconnectUser(logger, userCache, i);
				continue;
			}

			userCache[i].isBlocking = true;
			
			data.from = senderName;
			//Send common data
			SendCommonData(sender, data);
			sender.Close();
			userCache[i].isBlocking = false;
		}
	}
}

void AssignNewUser(DlgLogger& logger, CSocket * tmpSockPtr, CommonData &data, bool& end) {
	int userNameLen = data.fileSize;
	string id = data.message.substr(0, userNameLen);
	string pass = data.message.substr(userNameLen);
	int freeSlot = -1;
	CommonData respond;

	int exists = checkUser(id, pass);

	if (exists != LOGIN_NOT_EXIST) {
		respond.type = "dup";
		SendCommonData(*tmpSockPtr, respond);
		return;
	}

	ofstream fo;
	string path = "userdb/userInfo.txt";
	fo.open(path.data(), ios::app);
	fo << id << endl;
	fo << pass << endl;
	fo.close();

	//Send log to LogDisplay
	CString logLine;
	logLine += HANDLR;
	logLine += id.c_str();
	logLine += " Registered";
	SendLog(logger, logLine);

	respond.type = "suc";
	SendCommonData(*tmpSockPtr, respond);
}
	
int checkUser(string id, string pass) {
	ifstream fi;
	string path = "userdb\\userInfo.txt";
	string IdInDB;
	string PassInDB;
	fi.open(path.data());

	bool existID = false;
	if (!fi.is_open())
		return -2;

	while (!fi.eof()) {
		getline(fi, IdInDB);
		getline(fi, PassInDB);

		if (IdInDB == id)
			existID = true;

		if (IdInDB == id && PassInDB == pass)
			return LOGIN_SUC;
	}
	
	if (existID)
		return LOGIN_PASS_WRONG;
	else
		return LOGIN_NOT_EXIST;
}

void LogIn(DlgLogger& logger, CSocket * tmpSockPtr, CommonData &data, User *userCache, bool& end) {
	int userNameLen = data.fileSize;
	string id = data.message.substr(0, userNameLen);
	string pass = data.message.substr(userNameLen);
	UINT tmp;
	CommonData respond;
	CString logLine;
	logLine += HANDLR;
	logLine += id.c_str();

	int status = checkUser(id, pass);
	if (status == LOGIN_SUC) {
		for (size_t i = 0; i < UCACHE_LENGTH; i++)
			if (userCache[i].available) {
				userCache[i].inboxPort = stoi(data.from);
				tmpSockPtr->GetPeerName(userCache[i]._address, tmp);
				userCache[i].nickname = id;
				userCache[i].available = false;

				respond.type = "lisuc";
				SendCommonData(*tmpSockPtr, respond);

				//Send log to LogDisplay
				logLine += " Logged in";
				SendLog(logger, logLine);

				return;
			}
	}
	else {
		respond.type = "fail";
		SendCommonData(*tmpSockPtr, respond);
		logLine += " Attempt failed";
		SendLog(logger, logLine);
	}
}

void connectResolver(User * userCache, string& toScreen, int * portsStatus, bool& end, DlgLogger& logger) {
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

	CString cString =  RESOL;
	cString += " Started at port 1234";
	SendLog(logger, cString);

	while (!end)
	{
		mainServer.Listen();
		//tmpSockPtr = new CSocket();
		mainServer.Accept(tmpSock);
		
		errCode = mainServer.GetLastError();
		if (errCode > 0)
		{
			//cout << RESOL << " Errcode: " << errCode << endl;
			continue;
		}		
		

		UINT returnPort = 1234;
		for (int i = 0; i < SUBPORT_LENGTH; i++)
		{
			if (portsStatus[i] == PORT_STT_FREE)
			{
				returnPort += i + 1;
				tmpSock.Send(&returnPort, 4, 0);
				break;
			}
		}
		tempSlot.Close();
		tmpSock.Close();
		//delete tmpSockPtr;
	}
}

void receiveListener(int portNumber, int& portStatus, User * userCache, vector<Group> &groupCache, bool& end, DlgLogger& logger) {
	std::this_thread::sleep_for(100ms);
	CSocket subServer, sock;
	subServer.Create(portNumber);
	int messageLen = 0;
	string message;
	char buffer[255];
	CString clientAddress;
	UINT clientPort;
	CommonData data;
	
	CString cString = HANDLR;
	cString += " Started at port: ";
	cString += std::to_string(portNumber).data();
	SendLog(logger, cString);

	int stt = 0;
	char ch = ' ';
	while (!end)
	{
		stt = subServer.Listen();
		subServer.Accept(sock);
		portStatus = PORT_STT_BUSY;
		sock.GetPeerName(clientAddress, clientPort);
	
		ReceiveCommonData(sock, data);

		//Get userName
		string senderName = data.from;
		message = data.message;

		if (data.type == "re")
		{
			AssignNewUser(logger, &sock, data, end);
		}
		else if (data.type == "li") { //Log in
			LogIn(logger, &sock, data, userCache, end);
		}
		else if (data.type == "cg") { //create new Group
			createGroup(logger, data, groupCache, senderName);
			saveGroupCache(groupCache);

			//Phan hoi ve cho tat ca cac client trong group de tao group chat
			Group group = groupCache.back();
			CommonData respond;
			respond.type = "cg";
			respond.message = group.groupID;

			InboxGroup(logger, respond, group, userCache);
		}
		else if (data.type == "mu") //chat User -> User
		{
			InboxUsers(logger, senderName, data, userCache);
		}
		else if (data.type == "mg") {
			string groupID = data.to;
			int index = findGroup(groupID, groupCache);
			if (index == -1)
				return;
			Group group = groupCache[index];
			InboxGroup(logger, data, group, userCache);
		}
		// File sending
		else if (data.type == "fu")
		{
			//Recieve metadata of the file to transfer
			string fileName = data.message;
			ReceiveFile(logger, sock, data, senderName);
			
			//Send metadata of the file to user
			CommonData respond;
			respond.type = "fu"; 
			respond.from = senderName;
			respond.to = data.to;
			respond.message = respond.from + " sended a file " + fileName + " to you!";
			InboxUsers(logger, senderName, respond, userCache);
		}
		else if (data.type == "fg") { //send file to group
			string fileName = data.message;
			ReceiveFile(logger, sock, data, senderName);

			//Create metadata to send to group chat
			CommonData respond;
			respond.type = "fg";
			respond.from = senderName;
			respond.to = data.to;
			respond.message = respond.from + " sended a file " + fileName + " to the group!";

			//find group
			string groupID = data.to;
			int index = findGroup(groupID, groupCache);
			if (index == -1)
				return;

			Group group = groupCache[index];

			InboxGroup(logger, respond, group, userCache);
		}
		else if (data.type == "gf") { //get a file from chatbox
			SendFileUser(logger, sock, data);
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

bool IdInGroup(string id, Group group) {
	for (int i = 0; i < group.userInGroup.size(); i++) {
		if (id == group.userInGroup[i]) {
			return true;
		}
	}
	return false;
}

void InboxGroup(DlgLogger& logger, CommonData data, Group &group, User *userCache) {
	CSocket sender;
	bool status = false;
	CString logLine;
	logLine += CHATG;
	logLine += data.from.c_str();
	logLine += L" -> ";
	logLine += data.to.c_str();
	logLine += L" : ";
	logLine += data.message.c_str();
	SendLog(logger, logLine);
	for (int i = 0; i < UCACHE_LENGTH; i++) {
		if (!userCache[i].available && IdInGroup(userCache[i].nickname, group)) {
			sender.Create();
			status = sender.Connect(userCache[i]._address.GetString(), userCache[i].inboxPort);

			if (!status) {
				disconnectUser(logger, userCache, i);
				continue;
			}
			userCache[i].isBlocking = true;
			
			SendCommonData(sender, data);

			sender.Close();
			userCache[i].isBlocking = false;
		}
	}
}

void loadGroupCache(vector<Group> &group) {
	ifstream fi;
	fi.open("groupdb/groups.txt");

	if (!fi.is_open())
		return;

	while (!fi.eof()) {
		Group tmp;
		int n;
		char c;
		fi >> tmp.groupID >> c;
		if (tmp.groupID.length() < 1)
			break;
		fi >> n >> c;
		for (int i = 0; i < n; i++) {
			getline(fi, tmp.userInGroup[i]);
			tmp.userInGroup[i].erase(tmp.userInGroup[i].length() - 1);
		}
		group.push_back(tmp);
	}
	fi.close();
}

int findGroup(string groupID, vector<Group> &groupCache) {
	for (int i = 0; i < groupCache.size(); i++) {
		if (groupID == groupCache[i].groupID)
			return i;
	}
	return -1;
}

void createGroup(DlgLogger &logger, CommonData data, vector<Group> &groupCache, string senderName) {
	Group grp;
	grp.groupID = std::to_string(groupCache.size());
	string allUsers = data.message;
	vector<string> userNames;

	userNames.push_back(senderName);

	int start = 0;
	int end = 0;
	for (int i = 0; i < allUsers.length(); i++) {
		if (allUsers[i] == '\n') {
			end = i;
			string sub = allUsers.substr(start, end - start);
			start = end + 1;
			userNames.push_back(sub);
		}
	}
	grp.userInGroup = userNames;

	CString logLine;
	logLine += HANDLR;
	logLine += data.from.c_str();
	logLine += L" Create group ";
	logLine += grp.groupID.c_str();
	SendLog(logger, logLine);

	groupCache.push_back(grp);
}

void saveGroupCache(vector<Group> & groupCache) {

	ofstream fo;
	fo.open("groupdb/groups.txt");

	for (int i = 0; i < groupCache.size(); i++)
	{
		fo << groupCache[i].groupID << endl;
		fo << groupCache[i].userInGroup.size() << endl;
		for (int j = 0; j < groupCache[i].userInGroup.size(); j++)
		{
			fo << groupCache[i].userInGroup[j] << endl;
		}
	}

	fo.close();
}

void ReceiveFile(DlgLogger& logger, CSocket& sock, CommonData& data, string senderName){
	
	ofstream fo;
	string fPath = "./filedb/" + senderName + "_" + data.message;
	fo.open(fPath.data(),ios::binary);
	int toWriteSize = 0;

	CString logLine;
	logLine += FILHAN;
	logLine += data.from.c_str();
	logLine += L" Send a File ";
	logLine += data.message.c_str();
	logLine += L" to ";
	logLine += data.to.c_str();
	SendLog(logger, logLine);

	//Receive real file
	char Buffer[FILE_BUFFER_SIZE];
	while (toWriteSize > 0)
	{
		sock.Receive(&toWriteSize, sizeof(int), 0);
		sock.Receive(Buffer, toWriteSize, 0);
		fo.write(Buffer, toWriteSize);
	}
	fo.close();
}

void SendFileUser(DlgLogger& logger, CSocket& sock, CommonData& data) {
	string fPath = "./filedb/" + data.from + "_" + data.message;
	char Buffer[FILE_BUFFER_SIZE];

	CString logLine;
	logLine += FILHAN;
	logLine += data.to.c_str();
	logLine += L" Request a File ";
	logLine += data.message.c_str();
	logLine += L" Sent from ";
	logLine += data.from.c_str();
	SendLog(logger, logLine);

	FILE *fp = fopen(fPath.data(), "rb");

	if (fp == NULL) 
		return;

	int byteReaded = 0;

	do {
		byteReaded = fread(Buffer, 1, FILE_BUFFER_SIZE, fp);
		sock.Send(&byteReaded, sizeof(int), 0);
		sock.Send(Buffer, byteReaded);
	} while (byteReaded == FILE_BUFFER_SIZE);

	fclose(fp);
}


void userConnectionChecker(User* userCache, bool& end, DlgLogger& logger) {
	std::this_thread::sleep_for(150ms);
	CSocket checker;

	CString logLine;
	logLine = HANDLR;
	logLine += L"Connection Checker Started.";
	SendLog(logger, logLine);

	while (!end)
	{
		bool changed = false;
		std::this_thread::sleep_for(100ms);

		CommonData data;
		data.type = "updateUsers";

		for (int i = 0; i < UCACHE_LENGTH; i++)
		{
			if (userCache[i].available || userCache[i].isBlocking)
				continue;



			checker.Create();
			int status = checker.Connect(userCache[i]._address, userCache[i].inboxPort);
			if (!status)
			{
				disconnectUser(logger, userCache, i);
				changed = true;
				continue;
			}
			checker.Close();
		}

		/*if (changed) {
			CommonData data;
			data.type = "updateUsers";
			data.message = "";
			stringstream ss;
			for (int i = 0; i < UCACHE_LENGTH; i++) {
				if ()
			}
		}*/
	}
}

void disconnectUser(DlgLogger& logger, User* userCache, int index) {
	userCache[index].available = true;
	CString logLine;
	logLine += HANDLR;
	logLine += userCache[index].nickname.c_str();
	logLine += L" Disconnected.";
	SendLog(logger, logLine);
}