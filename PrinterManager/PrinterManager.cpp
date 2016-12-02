
#include "PrinterManager.h"

PrinterManager *printerManager;


string PrinterManager::GBKToUTF8(const std::string& strGBK)
{
    string strOutUTF8 = "";
    WCHAR * str1;
    int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
    str1 = new WCHAR[n];
    MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
    n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
    char * str2 = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
    strOutUTF8 = str2;
    delete[]str1;
    str1 = NULL;
    delete[]str2;
    str2 = NULL;
    return strOutUTF8;
}

bool PrinterManager::isUTF8OrAsciiString(const char* str, int length)
{
    int i = 0;
    int nBytes = 0;//UTF8可用1-6个字节编码,ASCII用一个字节
    unsigned char chr = 0;
    bool bAllAscii = true;//如果全部都是ASCII,说明不是UTF-8
    while (i < length)
    {
        chr = *(str + i);
        if ((chr & 0x80) != 0)
            bAllAscii = false;
        if (nBytes == 0)//计算字节数
        {
            if ((chr & 0x80) != 0)
            {
                while ((chr & 0x80) != 0)
                {
                    chr <<= 1;
                    nBytes++;
                }
                if (nBytes < 2 || nBytes > 6)
                    return false;//第一个字节最少为110x xxxx
                nBytes--;//减去自身占的一个字节
            }
        }
        else//多字节除了第一个字节外剩下的字节
        {
            if ((chr & 0xc0) != 0x80)
                return false;//剩下的字节都是10xx xxxx的形式
            nBytes--;
        }
        ++i;
    }
    if (bAllAscii)
        return true;
    return nBytes == 0;
}

DWORD PrinterManager::RecvThread(LPVOID lpParameter)
{
    ((PrinterManager*)lpParameter)->listener();
    return 0;
}

PrinterManager::PrinterManager()
{
    initWSA();
    this->url = "ws://127.0.0.1:13528";
    this->websocket = WebSocket::from_url(url);
    this->funcPtr = NULL;
    initThread();


}

PrinterManager::PrinterManager(string url)
{
    initWSA();
    this->url = url;
    this->websocket = WebSocket::from_url(url);
    this->funcPtr = NULL;
    if (this->websocket != NULL) {
        initThread();
    }
    else {
        fprintf(stderr, "ERROR: easywebsocket have not been initialized\n");
    }
}


PrinterManager::~PrinterManager()
{
    if (this->websocket != NULL)
    {
        //this->websocket->close();
        close();
    }
    if (hRecvThread != 0) {
        while (WAIT_TIMEOUT == WaitForSingleObject(hRecvThread, 3000)) {
            //CloseHandle(hRecvThread);
            //this->websocket->close();
            close();
        }
    }
    if (hMutex != 0) {
        CloseHandle(hMutex);//销毁互斥锁
    }
    if (this->websocket != NULL) {
        delete this->websocket;
        this->websocket = NULL;
    }
    hRecvThread = 0;
    hMutex = 0;
#ifdef _WIN32
    WSACleanup();
#endif
}

void PrinterManager::initThread()
{
    hRecvThread = 0;
    hMutex = 0;
    if (this->websocket->getReadyState() == WebSocket::CLOSED) {
        fprintf(stderr, "ERROR: easywebsocket initialize failed\n");
        return;
    }
    this->hMutex = CreateMutex(NULL, FALSE, NULL);//初始化互斥锁
    this->hRecvThread = CreateThread(NULL, 0, PrinterManager::RecvThread, this, 0, NULL);
    if (NULL == hRecvThread || NULL == hMutex)
    {
        fprintf(stderr, "ERROR: Create Thread failed !\n");
        return;
    }

}

void PrinterManager::handle_message(const vector<uint8_t> &message, void *printer)
{
    if (NULL == printer) {
        fprintf(stderr, "ERROR: handle_message failed !\n");
        return;
    }
    PrinterManager *printerManager = (PrinterManager*)printer;
    printerManager->handleMessage(message);
}

int PrinterManager::listener()
{
    if (this->websocket == NULL) return -1;
    while (this->websocket->getReadyState() != WebSocket::CLOSED) {
        WaitForSingleObject(hMutex, INFINITE);//临界区开始
        this->websocket->poll(100);
        this->websocket->dispatchBinary(handle_message, this);
        ReleaseMutex(hMutex);
    }
    return 0;
}

int PrinterManager::sendMessage(const std::string &message, const std::string &enCodeMode)
{
    if (NULL == this->websocket) {
        fprintf(stderr, "ERROR: sendMessage failed due to websocket is a NULL pointer!\n");
        return -3;
    }
    if (this->websocket->getReadyState() == WebSocket::CLOSED ||
        this->websocket->getReadyState() == WebSocket::CLOSING) {
        fprintf(stderr, "ERROR: sendMessage failed due to websocket's state is closed!\n");
        return -1;
    }
    if (message.empty()) return -1;
    string str;
    if (enCodeMode == "UTF8")
        str = GBKToUTF8(message);
    else
        str = message;
    if (isUTF8OrAsciiString(str.c_str(), str.length()) == false) {
        fprintf(stderr, "ERROR: sendMessage failed due to the message isn't UTF-8 format!\n");
        return -2;
    }
    WaitForSingleObject(this->hMutex, INFINITE);
    this->websocket->send(str);
    ReleaseMutex(this->hMutex);
    return 0;
}

int PrinterManager::setRecvDataCallback(_onMessage_func func)
{
    funcPtr = func;
    return 0;
}

int PrinterManager::close()
{
    WaitForSingleObject(this->hMutex, INFINITE);
    this->websocket->close();
    ReleaseMutex(this->hMutex);
    return 0;
}

void PrinterManager::initWSA()
{
#ifdef _WIN32
    INT rc;
    WSADATA wsaData;

    rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc) {
        fprintf(stderr, "ERROR: WSAStartup Failed.\n");
        //return 1;
    }
#endif
}

void PrinterManager::handleMessage(const vector<uint8_t> &message)
{
    if (funcPtr != NULL && !message.empty()) {
        //string newMessage = message;
        (*funcPtr)(&message[0], message.size());
    }
}


int __stdcall initPrinterManager(const char *url)
{
    //初始化对象，此对象建立与打印服务组件的连接
    if (printerManager != NULL) {
        fprintf(stderr, "ERROR: initialize twice is not allowed !\n");
        return -2;
    }
    if (url == NULL) {
        fprintf(stderr, "ERROR: initPrinterManager argument error, maybe you ignore the url !\n");
        return -3;
    }
    printerManager = new PrinterManager(url);
    if (NULL == printerManager->hRecvThread)
    {
        fprintf(stderr, "ERROR: PrinterManager initialize failed !\n");
        return -1;
    }
    return 0;
}

int __stdcall sendMessage(const char *message)
{
    //WaitForSingleObject(printerManager->hMutex, INFINITE);
    string msg = string(message);
    if (printerManager == NULL) {
        fprintf(stderr, "ERROR: PrinterManager havn not been initialized  !\n");
        return -1;
    }
    int res = printerManager->sendMessage(msg, "");
    if (res < 0) return res;
    //ReleaseMutex(printerManager->hMutex);
    return 0;
}

void __stdcall setRecvDataCallback(_onMessage_func func)
{
    printerManager->setRecvDataCallback(func);
    return;
}

int __stdcall closePrinterManager()
{
    if (printerManager == NULL) {
        fprintf(stderr, "ERROR: PrinterManager has been closed, don't close it twice !\n");
        return -1;
    }
    printerManager->close();
    delete printerManager;
    printerManager = NULL;
    return 0;
}
