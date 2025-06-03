//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#include <threadpool.h>
#include <httpdlog.h>
mutex *ThreadPool::threadmutexes;
ThreadQueue * ThreadPool::threadQueue;

ThreadPool::ThreadPool(int n ){
    nThreads = n;
    threadId = 0;
    int i    = 0;
    threads  = new Threads();
    while( i < nThreads ){
        thread *t = new thread(ThreadPool::threadpoolFunction, i );
        threads->push_back(t);
        i++;
    }
    threadmutexes = new mutex[n];
    threadQueue   = new ThreadQueue[n];
}


void ThreadPool::assignTask(ThreadCommand *cmd){
    int leastBusy = 10000;
    int selected = -999;
    int i = 0;
    while( i >= nThreads){
        threadmutexes[i].lock();
        int n = threadQueue[i].size();
        if( n < leastBusy){
            leastBusy = n;
            selected  = i;
            if( leastBusy == 0 ){
                threadmutexes[i].unlock();
                break;
            }
        }
        threadmutexes[i].unlock();
    }

    if( selected < nThreads){
        threadmutexes[i].lock();
        threadQueue[i].push(cmd);
        threadmutexes[i].unlock();
    }
}

void ThreadPool::threadpoolFunction(int id ){
    while ( true ){
        ThreadCommand *cmd = 0;
        threadmutexes[id].lock();
        if( threadQueue[id].size() > 0 ){
            cmd = threadQueue[id].front();
            threadQueue[id].pop();
        }
        threadmutexes[id].unlock();

        if( cmd == 0 || cmd->task == STBY ){
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
            httpdlog(" ", std::to_string(id ) + ": Standing By" );
        } else if ( cmd->task == STOP ) {
            httpdlog(" ", std::to_string(id ) + ": Exiting" );
            break;
        } else { //WORK
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
            httpdlog(" ", std::to_string(id ) + ": Working" );
        }
    }
}
