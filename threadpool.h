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
        SSL *ssl;
};


typedef queue<ThreadCommand*> ThreadQueue;
typedef vector<thread*> Threads;

#define MAXTHREADS 256

class ThreadPool{

private:
    int nThreads;
    int threadId;
    int threadTaskCount[MAXTHREADS];
    int threadTaskDoneCount[MAXTHREADS];

    Threads *threads;
    static mutex   *threadmutexes;
    static ThreadQueue *threadQueue;

public :

    ThreadPool(int );
    ~ThreadPool();

    void createPool(int n );
    void assignTask (ThreadCommand *cmd );
    void assignTaskRr (ThreadCommand *cmd );
    void taskDone(int i );
    static size_t  isFullHeaderPresent(char *data, size_t len);
    static void threadpoolFunction(int id);


};


#endif // THREADPOOL_H_INCLUDED
