#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

class ThreadPool{
public :

    ThreadPool();
    ~ThreadPool();

    void threadPoolFunction(int threadid, SOCKET fd){
    }

    bool assignTask ();
};


#endif // THREADPOOL_H_INCLUDED
