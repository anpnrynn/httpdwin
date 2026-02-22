//Copyright Anoop Kumar Narayanan  -2025 //httpdwin
#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#include <iostream>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <malloc.h>
#include <version.h>
#include <chrono>
#include <queue>
#include <vector>
#include <mutex>



#include <openssl/ssl.h>
#include <openssl/err.h>


#include <httpresponse.h>
#include <httprequest.h>
#include <cookie.h>

using namespace std;


enum Task{
    STBY  = 1,
    WORK  = 2,
    STOP  = 3
};

class ThreadCommand{
    public:
        ThreadCommand( );
        ThreadCommand( Task _task, SOCKET _socket, bool _isSsl, bool _isSslaccepted, bool _ipv6, int _p, string address, SSL *_ssl);
        ~ThreadCommand( );
        Task task;
        SOCKET fd;
        bool isSsl;
        bool isSslAccepted;
        bool isIpv6;
        int  port;
        string ipAddress;
        string portStr;
        SSL *ssl;
};


typedef queue<ThreadCommand*> ThreadQueue;
typedef vector<thread*> Threads;

#define MAXTHREADS 256


class ThreadInfo {
public:
    ThreadCommand *cmd;
    HttpRequest   *req;
    HttpResponse  *resp;
};


class ThreadPool{

private:
    int nThreads;
    int threadId;
    int threadTaskCount[MAXTHREADS];
    int threadTaskDoneCount[MAXTHREADS];

    Threads *threads;
    static mutex   *threadmutexes;
    static ThreadQueue *threadQueue;

    static mutex    tempFileMutex;
    static uint64_t tempNum;

    static void   getTempFileName(string &tempFileName);
    static void   writeToTempFile(string tempFileName, ThreadCommand* cmd, HttpRequest* req, size_t dataStart, int threadId );

public :

    ThreadPool(int );
    ~ThreadPool();

    //void createPool(int n );
    void assignTask (ThreadCommand *cmd );
    void assignTaskRr (ThreadCommand *cmd );
    void taskDone(int i );
    static size_t  isFullHeaderPresent(char *data, size_t len);
    static void threadpoolFunction(int id);
    static void simpleChunkedResponse(int id, ThreadCommand* cmd, HttpResponse* resp, const char* simplestring);
    static void sendHttpDataFinal(char* data, size_t len);
    static void sendHttpData(char* data, size_t len);
    static void sendHttpHeader();
    static void setCookie(string name, string value, string expires, uint64_t maxAge, bool secure, bool httpOnly, std::string path, std::string domain);
    static void delCookie(string name);
    static string generateJsonFile();
    static void addHttpHeader(string value);
    static void clearHttpSession();
};


#endif // THREADPOOL_H_INCLUDED
