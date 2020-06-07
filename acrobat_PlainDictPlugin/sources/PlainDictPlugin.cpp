/*********************************************************************
 ADOBE SYSTEMS INCORPORATED. Copyright (C) 2020 KnIfER
 Whatever The Liscence (WTL™).
 Copyright (C) 1998-2006 Adobe Systems Incorporated
 All rights reserved.

 NOTICE: Adobe permits you to use, modify, and distribute this file
 in accordance with the terms of the Adobe license agreement
 accompanying it. If you have received this file from a source other
 than Adobe, then your use, modification, or distribution of it
 requires the prior written permission of Adobe.

 -------------------------------------------------------------------*/
/** 
\ based on BasicPlugin.cpp

*********************************************************************/

#include <iostream>
#include <sstream>
#include <string>

// Acrobat Headers.
#ifndef MAC_PLATFORM
#include "PIHeaders.h"
#endif

#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#endif

#include <json.hpp>
using nlohmann::json;


/*-------------------------------------------------------
	Constants/
	Declarations
	.
-------------------------------------------------------*/

static int capacity=1024;
static char* buffer = (char*)malloc(capacity);
static int length;
boolean start;
struct sockaddr_in address;
struct hostent *server;
int WSA=-1;


char host[128]={0};

int port=8080;

int sharetype;

int desiredSlot=1;

int verbose;

int timeout=1;

const char* MyPluginExtensionName = "ADBE:PlainDictPlugin";

/* A convenient function to add a menu item for your plugin. */
ACCB1 ASBool ACCB2 PluginMenuItem(const char* MyMenuItemTitle, const char* MyMenuItemName);

/* Callback on each run of the text selection. */
static ACCB1 ASBool ACCB2 PDTextSelectEnumTextProcCB (void* procObj, PDFont font, ASFixed size, PDColorValue color, char* text, ASInt32 textLen);

/*-------------------------------------------------------
	Functions.
	MyPluginCommand is the function to be called when executing a menu,
	The entry point for user's code, just add your code inside.
r-------------------------------------------------------*/

/* MyPluginSetmenu Function to set up menu for the plugin. It calls a convenient function PluginMenuItem.
** @return true if successful, false if failed. */
ACCB1 ASBool ACCB2 MyPluginSetmenu()
{
	return PluginMenuItem("Plain Dictionary", MyPluginExtensionName); 
}

static ACCB1 ASBool ACCB2 PDTextSelectEnumTextProcCB(void* procObj, PDFont font, ASFixed size, PDColorValue color, char* text, ASInt32 textLen)
{
	int selen = strlen(text);
	//char str[256];
	//sprintf(str," %s %d \n", text, selen);
	int nowCap = capacity;
	if(length+selen+10>nowCap) { // 扩 容
		int elta = nowCap*2;
		if(elta>8192) elta=8192;
		int estimate = elta+nowCap;
		if(estimate<128*1024){
			char* buffer_new = (char*)malloc(capacity=estimate);
			memcpy(buffer_new, buffer, nowCap);
			free(buffer);
			buffer = buffer_new;
		} else {
			return false;
		}
	}
	//start?"%s%s":"%s %s"
	length=sprintf(buffer, "%s%s", buffer, text);
	start=0;
	//sprintf(buffer,"%s", text);
	//AVAlertNote(buffer);
	return true;
}

void GetText() {
	// try to get front PDF document 
	AVDoc avDoc = AVAppGetActiveDoc();

	if (avDoc == NULL) return;

	PDTextSelect ts = static_cast<PDTextSelect>(AVDocGetSelection(avDoc));

	// get this plugin's name for display

	ASAtom NameAtom = ASExtensionGetRegisteredName (gExtensionID);

	const char * name = ASAtomGetString(NameAtom);


	PDTextSelectEnumTextProc cbPDTextSelectEnumTextProc = ASCallbackCreateProto(PDTextSelectEnumTextProc, &PDTextSelectEnumTextProcCB);


	PDTextSelectEnumText(ts, cbPDTextSelectEnumTextProc, NULL);

	//if(verbose) {
	//	AVAlertNote("Content too long!");
	//}

	//PDDoc pdDoc = AVDocGetPDDoc (avDoc);

	//sprintf(buffer,"%s\n\nTotal Page Count : %d %d", buffer, PDDocGetNumPages (pdDoc), length);

	//AVAlertNote(buffer);
}

char* textFileRead(FILE* file) {
	char* text;
	fseek(file,0,SEEK_END);
	long lSize = ftell(file);
	text=(char*)malloc(lSize+1);
	rewind(file); 
	fread(text,sizeof(char),lSize,file);
	text[lSize] = '\0';
	return text;
}

void RunOnTextSelection(int sendTo){
	if (WSA != 0) {
		WSADATA wsaData;
		WSA = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (WSA != 0) {
			return;
		}
	}

	// todo 从Chrome书签取值

	address.sin_addr.s_addr = inet_addr(host);
	address.sin_family = AF_INET;
	address.sin_port = htons(port); 

	if(verbose>2) {
		AVAlertNote(host);
	}

	server = gethostbyaddr((char *)&address.sin_addr, 4, AF_INET);

	if(server) {
		if(verbose>2) {
			AVAlertNote(server->h_name);
		}
		SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

		int iMode = 1;  

		ioctlsocket(sockClient, FIONBIO, (u_long FAR*)&iMode);  // 设置为非阻塞的socket  

		int error=0;

		if(-1 == connect(sockClient,(SOCKADDR *)&address,sizeof(SOCKADDR))) {
			//printf("尝 试 去 连 接 服 务 端 \n ");
			struct timeval time{timeout, 0}; // 超 时 时 间  

			fd_set set;  
			FD_ZERO(&set);  
			FD_SET(sockClient, &set);  

			error = 1;

			if (select(-1, NULL, &set, NULL, &time) > 0) {
				int optLen = sizeof(int);  
				getsockopt(sockClient, SOL_SOCKET, SO_ERROR, (char*)&error, &optLen);   
			}  
		}

		if(error) {
			if(verbose) {
				AVAlertNote("Time out !");
			}
		} else { 
			start=1;

			memset(buffer, length=0, 1);

			if(sendTo==1 && sharetype==1){
				sendTo=2;
			}

			length = sprintf(buffer,"POST /PLOD/&f=%d HTTP/1.0\r\nHost: %s\r\nUser-Agent: Mozilla/5.0\r\nContent-Type:application/x-www-form-urlencoded\r\nContent-Length: -1\r\nConnection:close\r\n\r\n", sendTo, host);

			GetText();

			iMode = 0; 

			ioctlsocket(sockClient, FIONBIO, (u_long FAR*)&iMode);  // 设置为阻塞socket 
#ifdef WIN32
			send(sockClient, buffer, length, 0);
#else
			write(sockClient,request.c_str(),request.size());
#endif
			if(verbose>1) {
				AVAlertNote("done!");
			}
		}

#ifdef WIN32
		closesocket(sockClient);
#else
		close(sockClient);
#endif
	} else {
		if(verbose) {
			AVAlertNote("Server not found !");
		}
	}
}

ACCB1 void ACCB2 MyPluginCommand(void *clientData)
{
	if(clientData){
		//AVAlertNote((char*)clientData);
		char code = *((char*)clientData);
		switch(code){
			case '1':
				RunOnTextSelection(1);
			break;
			case '2':
				RunOnTextSelection(2);
			break;
			case '3':
			break;
		}
	}
}

/* MyPluginIsEnabled Function to control if a menu item should be enabled.
** @return true to enable it, false not to enable it.
*/
ACCB1 ASBool ACCB2 MyPluginIsEnabled(void *clientData)
{
	//return (AVAppGetActiveDoc() != NULL); // enabled only if there is a open PDF document. 
	return true; // always enabled.
}