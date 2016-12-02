#ifndef _DLL_PRINTERMANAGER_H
#define _DLL_PRINTERMANAGER_H
//#define DLL_PRINTERMANAGER_API __declspec(dllexport)

#pragma once
#include <iostream>
#include <WinSock2.h>
#include <assert.h>
#include <stdio.h>
#include <string>
#include "easywsclient/easywsclient.hpp"

using namespace std;
using easywsclient::WebSocket;
extern "C" typedef void(__stdcall *_onMessage_func)(const char* message);


extern "C" int  __stdcall initPrinterManager(const char *url);
extern "C" void __stdcall setRecvDataCallback(_onMessage_func func);
extern "C" int  __stdcall sendMessage(const char *message);
extern "C" int  __stdcall closePrinterManager();

class PrinterManager
{
private:
    WebSocket::pointer websocket;//websocket����
    string url;//���ӡ����������ʹ�õ�URL
    _onMessage_func funcPtr;//�ص�����ָ��

public:
    HANDLE hRecvThread;//�����߳̾��
    HANDLE hMutex;//�˻��������ڷ�ֹwebsocket�еĶ�д�������Ķ��߳����ݲ���ȫ

public:
    PrinterManager();
    PrinterManager(string url);
    ~PrinterManager();

    ///����Ϊ�ӿں���
    /*��������
    message��ʾҪ���͵��ַ���
    enCodeMode��ʾ���뷽ʽ
    */
    int sendMessage(const std::string &message, const std::string &enCodeMode = "UTF8");

    //�趨�ص�������funcΪ�ص�����ָ��
    int setRecvDataCallback(_onMessage_func func);
    int close();

private:
    //��ʼ��wsa
    void initWSA();

    //���������߳���ʼ������lpParameterΪ
    static DWORD WINAPI RecvThread(LPVOID lpParameter);
    void initThread();//��ʼ���߳�

    //�����������ݣ����յ�������ת��handle_Message��������	
    int listener();
    //�����еľ�̬handle_Message�����������printerָ����ò�ͬ�����handleMessage����
    static void handle_message(const std::string & message, void *printer = NULL);
    //����Ľ��պʹ������ݵĺ���������ִ�лص�����
    void handleMessage(const std::string &message);
    //��GBK����תΪUTF8����
    static string GBKToUTF8(const std::string& strGBK);
    //�ж��Ƿ���UTF8String��ȫASCII���룬strΪ����ָ�룬lengthΪ���鳤��
    static bool isUTF8OrAsciiString(const char* str, int length);

};

#undef DLL_PRINTERMANAGER_API

#endif

