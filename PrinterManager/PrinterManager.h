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
    WebSocket::pointer websocket;//websocket对象
    string url;//与打印服务建立连接使用的URL
    _onMessage_func funcPtr;//回调函数指针

public:
    HANDLE hRecvThread;//接收线程句柄
    HANDLE hMutex;//此互斥锁用于防止websocket中的读写缓冲区的多线程数据不安全

public:
    PrinterManager();
    PrinterManager(string url);
    ~PrinterManager();

    ///以下为接口函数
    /*发送数据
    message表示要发送的字符串
    enCodeMode表示编码方式
    */
    int sendMessage(const std::string &message, const std::string &enCodeMode = "UTF8");

    //设定回调函数，func为回调函数指针
    int setRecvDataCallback(_onMessage_func func);
    int close();

private:
    //初始化wsa
    void initWSA();

    //接收数据线程起始函数，lpParameter为
    static DWORD WINAPI RecvThread(LPVOID lpParameter);
    void initThread();//初始化线程

    //监听接收数据，如收到数据则转到handle_Message函数处理	
    int listener();
    //对象共有的静态handle_Message处理，它会根据printer指针调用不同对象的handleMessage函数
    static void handle_message(const std::string & message, void *printer = NULL);
    //对象的接收和处理数据的函数，包含执行回调函数
    void handleMessage(const std::string &message);
    //将GBK编码转为UTF8编码
    static string GBKToUTF8(const std::string& strGBK);
    //判断是否是UTF8String或全ASCII编码，str为数组指针，length为数组长度
    static bool isUTF8OrAsciiString(const char* str, int length);

};

#undef DLL_PRINTERMANAGER_API

#endif

