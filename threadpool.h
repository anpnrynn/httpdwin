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
        ThreadCommand( Task t, SOCKET s, bool ssl =false, bool sslaccepted =false, bool ipv6 = false, int p = -1, string address = "", SSL *_ssl = 0 );

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

class ThreadPool{

private:
    int nThreads;
    int threadId;
    Threads *threads;
    static mutex   *threadmutexes;
    static ThreadQueue *threadQueue;

public :

    ThreadPool(int );
    ~ThreadPool();

    void createPool(int n );
    void assignTask (ThreadCommand *cmd );
    static void threadpoolFunction(int id);


};


#endif // THREADPOOL_H_INCLUDED
