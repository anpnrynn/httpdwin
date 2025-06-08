//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#include <threadpool.h>
#include <httpdlog.h>
mutex *ThreadPool::threadmutexes;
ThreadQueue * ThreadPool::threadQueue;


ThreadCommand::ThreadCommand( ){
    task = STBY;
    fd = 0;
    isSsl=false;
    isSslAccepted=false;
    isIpv6=false;
    int port = -1;
    ipAddress ="";
    ssl = 0;
}

ThreadCommand::ThreadCommand( Task t, SOCKET s, bool isssl, bool sslaccepted, bool ipv6, int p, string address, SSL* _ssl){
    task = t;
    fd = s;
    isSsl = isssl;
    isSslAccepted = sslaccepted;
    isIpv6 = ipv6;
    port = p;
    ipAddress = address;
    ssl = _ssl;
}

ThreadPool::ThreadPool(int n ){
    nThreads = n;
    threadmutexes = new mutex[n];
    threadQueue   = new ThreadQueue[n];
    threadId = 0;
    int i    = 0;
    threads  = new Threads();
    while( i < nThreads ){
        thread *t = new thread(ThreadPool::threadpoolFunction, i );
        threads->push_back(t);
        i++;
    }
}

ThreadPool::~ThreadPool(){
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
        httpdlog("INFO", std::to_string(i ) + ": Thread assigned the task" );
        threadQueue[i].push(cmd);
        threadmutexes[i].unlock();
    }
}


int ThreadPool::isFullHeaderPresent( char *data, int len ){
    int i = 0;
    char ch = 0;
    while( i < len ){
        ch = data[i];
        if( ch == '\r')
        {
            if( data[i+1] == '\n' && data[i+2] == '\r' && data[i+3] == '\n' ){
                httpdlog("INFO","isFullHeaderPresent: Data starts here : "+std::to_string(i+4));
                return i+4;
            }
        }
        i++;
    }
    return 0;
}

void ThreadPool::threadpoolFunction(int id ){
    bool standby = true;
    bool dataNotReceived = true;
    httpdlog("INFO", std::to_string(id ) + ": Thread starting up" );
    while ( true ){
        ThreadCommand *cmd = 0;
        threadmutexes[id].lock();
        if( threadQueue[id].size() > 0 ){
            cmd = threadQueue[id].front();
            threadQueue[id].pop();
        }
        threadmutexes[id].unlock();



        if( cmd == 0 || cmd->task == STBY ){
            std::this_thread::sleep_for(std::chrono::microseconds(1000) );
            if( standby ){
                httpdlog("INFO", std::to_string(id ) + ": Standing By" );
                standby = false;
            }
            continue;

        } else if ( cmd->task == STOP ) {
            std::this_thread::sleep_for(std::chrono::microseconds(10) );
            httpdlog("INFO", std::to_string(id ) + ": Exiting" );
            break;
        } else { //WORK
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
            httpdlog("INFO", std::to_string(id ) + ": Working " + (cmd->isSsl?"SSL":"Non-SSL") + (cmd->isIpv6?" - IPv6":" - IPv4")  );
            if( cmd->isSsl ){
                httpdlog("INFO",std::to_string(id ) + ": SSL connection received" + (cmd->isIpv6?" - IPv6 ":" - IPv4 ") + std::to_string( (unsigned long long int)cmd->ssl));
                if( cmd->isSslAccepted ){
                    HttpRequest *req = new HttpRequest();
                    size_t nBytes = 0;
                    int rc = 0;
                    while( nBytes <= 0 ){
                        rc = SSL_read_ex ( cmd->ssl, req->m_Buffer, MAXBUFFER - req->m_Len, &nBytes );
                        if( nBytes > 0 ){
                            httpdlog( "INFO",std::to_string(id ) + ": Received SSL data " + std::to_string(nBytes ) + (cmd->isIpv6?" - IPv6":" - IPv4"));
                            req->m_Len += nBytes;
                            req->m_Buffer[req->m_Len] = 0;
                            httpdlog("INFO", (char *)req->m_Buffer );
                            nBytes = 0;

                            int hLen = 0;
                            HttpRequest::readHttpHeader ( req, (char *)(req->m_Buffer), &hLen, req->m_Len );

                        } else if ( nBytes == 0 ){
                            std::this_thread::sleep_for(std::chrono::microseconds(10) );
                        } else {
                            if( dataNotReceived ){
                                httpdlog("INFO",std::to_string(id ) + ": Data not received "  + (cmd->isIpv6?" - IPv6":" - IPv4") );
                                dataNotReceived = false;
                            }
                            std::this_thread::sleep_for(std::chrono::microseconds(10));
                        }
                    }
                } else {
                    httpdlog( "INFO", std::to_string(id ) + ": Connection not accepted" + (cmd->isIpv6?" - IPv6":" - IPv4"));
                }

            } else {
                httpdlog("INFO", std::to_string(id ) + ": Working " + (cmd->isSsl?"SSL":"Non-SSL") + (cmd->isIpv6?" - IPv6":" - IPv4")  );
                HttpRequest *req = new HttpRequest();
                int dataStartPresent = 0;
                int dataStart = 0;
                int nBytes = 0;
                req->m_Len = 0;
                while( true ){
                    nBytes = recv ( cmd->fd, (char *)req->m_Buffer, MAXBUFFER-req->m_Len, 0 );
                    if( nBytes > 0 ){
                        dataStartPresent   = 0;
                        dataStartPresent   = ThreadPool::isFullHeaderPresent((char*)&(req->m_Buffer[req->m_Len]), nBytes );
                        req->m_Len += nBytes;
                        if( dataStartPresent == 0 ){
                            dataStart += nBytes;
                        } else {
                            dataStart += dataStartPresent;
                            break;
                        }
                    } else if ( nBytes < 0 ){
                        httpdlog("INFO", std::to_string(id ) + ": Received negative bytes : " + std::to_string(nBytes) );
                        //dataStart = 0;
                        break;
                    }
                }
                int hLen = 0;
                HttpRequest::readHttpHeader ( req, (char *)(req->m_Buffer), &hLen, req->m_Len );
                httpdlog("INFO", std::to_string(id ) + ": Data starts from : " + std::to_string(dataStart) + " Total Read: " + std::to_string(req->m_Len) );

                bool hasDataBeenRead = false;
                if( req->m_Len > dataStart ){
                    //Write to file, possible post data or multipart post data or put data
                }
                hasDataBeenRead = true;

                if( req->m_Len == dataStart || hasDataBeenRead ){
                    //No extra data like post or post multipart or put data
                    HttpResponse *resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                    httpdlog("INFO", std::to_string(id ) + ": Building response header " );
                    resp->BuildResponseHeader(0);
                    httpdlog("INFO", std::to_string(id ) + ": Built response header " );
                    resp->addResponseData(resp->m_HttpData);
                    httpdlog("INFO", std::to_string(id ) + ": Adding data " );

                    int nHttpDataBytes = strlen( (char *)resp->m_Buffer );

                    httpdlog("INFO",  std::to_string(id )+": " + (char *)resp->m_Buffer );

                    int partial = 0, n =0;
                    do {
                        n =  send ( cmd->fd, (char *)&(resp->m_Buffer[partial]), nHttpDataBytes, 0 );
                        if( n > 0 ){
                            partial += n;
                        } else if( n == -1 ){
                            break;
                        } else {
                        }
                    }while ( partial < nHttpDataBytes );
                }
                //Extra read part starts here
                //Write part starts here
            }
        }
    }
}
