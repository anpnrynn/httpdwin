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


using namespace std;


enum Task{
    STBY  = 1,
    WORK  = 2,
    STOP  = 3
};

class ThreadCommand{
    public:
        ThreadCommand( ){ task = STBY; fd = 0; }
        ThreadCommand( ThreadCommand &cmd ){ task = cmd.task ; fd = cmd.fd ; }
        ThreadCommand( Task t, SOCKET s){ task = t; fd = s; }
        Task task;
        SOCKET fd;

        ThreadCommand & operator = (ThreadCommand & a){
            task = a.task;
            fd = a.fd;
            return (*this);
        }
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
