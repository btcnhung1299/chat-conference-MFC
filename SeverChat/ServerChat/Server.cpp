// Project1.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "Server.h"
#include "CommonLib.h"
#include <afxsock.h>
#include <conio.h>
#include <Windows.h>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define LOGIN_SUC 0
#define LOGIN_PASS_WRONG 1
#define LOGIN_NOT_EXIST 2

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

void InboxUsers(string senderName, CommonData &data, User * userCache) {
	CSocket sender;
	bool status = false;
	string toName = data.to;

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
				userCache[i].available = true;
				continue;
			}
			
			data.from = senderName;

			//Send common data
			SendCommonData(sender, data);

			sender.Close();
		}
	}
}

void AssignNewUser(CDialog& dlg, CSocket * tmpSockPtr, CommonData &data, bool& end) {
	int userNameLen = data.fileSize;
	string id = data.message.substr(0, userNameLen);
	string pass = data.message.substr(userNameLen);
	int freeSlot = -1;
	CommonData respond;

	int exists = checkUser(dlg, id, pass);

	if (exists != LOGIN_NOT_EXIST) {
		respond.type = "dup";
		SendCommonData(*tmpSockPtr, respond);
		return;
	}

	ofstream fo;
	string path = "./userdb/userInfo.txt";
	fo.open(path);
	fo << id << endl;
	fo << pass << endl;
	fo.close();

	respond.type = "suc";
	SendCommonData(*tmpSockPtr, respond);
}
	
int checkUser(CDialog& dlg, string id, string pass) {
	ifstream fi;
	string path = "./userdb/userInfo.txt";
	string IdInDB;
	string PassInDB;
	fi.open(path);

	bool existID = false;

	while (!fi.eof()) {
		getline(fi, IdInDB);
		IdInDB.erase(IdInDB.length() - 1);
		getline(fi, PassInDB);
		PassInDB.erase(PassInDB.length() - 1);

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

void LogIn(CDialog& dlg, CSocket * tmpSockPtr, CommonData &data, User *userCache, bool& end) {
	int userNameLen = data.fileSize;
	string id = data.message.substr(0, userNameLen);
	string pass = data.message.substr(userNameLen);
	UINT tmp;
	CommonData respond;

	int status = checkUser(dlg, id, pass);
	if (status == LOGIN_SUC) {
		for (size_t i = 0; i < UCACHE_LENGTH; i++)
			if (userCache[i].available) {
				userCache[i].inboxPort = stoi(data.from);
				tmpSockPtr->GetPeerName(userCache[i]._address, tmp);
				userCache[i].nickname = id;
				userCache[i].available = false;

				respond.type = "lisuc";
				SendCommonData(*tmpSockPtr, respond);

				return;
			}
	}
	else {
		respond.type = "fail";
		SendCommonData(*tmpSockPtr, respond);
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

void receiveListener(int portNumber, int& portStatus, User * userCache, vector<Group> &groupCache, bool& end, bool& coutBlocked, CDialog& dlg) {
	std::this_thread::sleep_for(100ms);
	CSocket subServer, sock;
	subServer.Create(portNumber);
	int messageLen = 0;
	string message;
	char buffer[255];
	CString clientAddress;
	UINT clientPort;
	CommonData data;


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
	
		ReceiveCommonData(sock, data);

		//Get userName
		string senderName = data.from;
		message = data.message;

		if (data.type == "re")
		{
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			AssignNewUser(dlg, &sock, data, end);
			coutBlocked = false;
		}
		else if (data.type == "li") { //Log in
			//Wait until screen is free
			while (coutBlocked) 
			{
			}
			coutBlocked = true;
			LogIn(dlg, &sock, data, userCache, end);
			coutBlocked = false;
		}
		else if (data.type == "cg") { //create new Group
			createGroup(dlg, data, groupCache);
			saveGroupCache(groupCache);
		}
		else if (data.type == "mu") //chat User -> User
		{
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			InboxUsers(senderName, data, userCache);
			coutBlocked = false;
		}
		else if (data.type == "mg") {
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			string groupID = data.to;
			int index = findGroup(groupID, groupCache);
			if (index == -1)
				return;

			Group group = groupCache[index];
			InboxGroup(data, group, userCache);
			coutBlocked = false;
		}
		// File sending
		else if (data.type == "fu")
		{
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			//Recieve metadata of the file to transfer
			string fileName = data.message;
			ReceiveFile(dlg, sock, data, senderName);
			
			//Send metadata of the file to user
			CommonData respond;
			respond.type = "fu"; 
			respond.from = senderName;
			respond.to = data.to;
			respond.message = respond.from + " sended a file " + fileName + " to you!";
			InboxUsers(senderName, respond, userCache);
			coutBlocked = false;
		}
		else if (data.type == "fg") { //send file to group
			//Wait until screen is free
			while (coutBlocked)
			{
			}
			coutBlocked = true;
			string fileName = data.message;
			ReceiveFile(dlg, sock, data, senderName);

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

			InboxGroup(respond, group, userCache);
			coutBlocked = false;
		}
		else if (data.type == "gf") { //get a file from chatbox
			SendFileUser(sock, data);
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

void InboxGroup(CommonData data, Group &group, User *userCache) {
	CSocket sender;
	bool status = false;

	for (int i = 0; i < UCACHE_LENGTH; i++) {
		if (!userCache[i].available && IdInGroup(userCache[i].nickname, group)) {
			sender.Create();
			status = sender.Connect(userCache[i]._address.GetString(), userCache[i].inboxPort);

			if (!status) {
				userCache[i].available = true;
				continue;
			}
			
			SendCommonData(sender, data);

			sender.Close();
		}
	}
}

void loadGroupCache(vector<Group> &group) {
	ifstream fi;
	fi.open("./groupdb/groups.txt");

	while (!fi.eof()) {
		Group tmp;
		int n;
		fi >> tmp.groupID;
		fi >> n;
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

void createGroup(CDialog &dlg, CommonData data, vector<Group> &groupCache) {
	Group grp;
	grp.groupID = groupCache.size();
	string allUsers = data.message;
	vector<string> userNames;
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

	groupCache.push_back(grp);
}

void saveGroupCache(vector<Group> & groupCache) {

	ofstream fo;
	fo.open("./groupdb/groups.txt");

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

void ReceiveFile(CDialog& dlg, CSocket& sock, CommonData& data, string senderName){
	
	ofstream fo;
	string fPath = "./filedb/" + senderName + "_" + data.message;
	fo.open(fPath.data(),ios::binary);
	int toWriteSize = 0;

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

void SendFileUser(CSocket& sock, CommonData& data) {
	string fPath = "./filedb/" + data.from + "_" + data.message;
	char Buffer[FILE_BUFFER_SIZE];
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