//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#define _CRT_SECURE_NO_WARNINGS
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

ThreadCommand::ThreadCommand( Task _task, SOCKET _socket, bool _isSsl, bool _isSslAccepted, bool _ipv6, int _p, string _address, SSL* _ssl){
    task = _task;
    fd = _socket;
    isSsl = _isSsl;
    isSslAccepted = _isSslAccepted;
    isIpv6 = _ipv6;
    port = _p;
    ipAddress = _address;
    ssl = _ssl;
}

ThreadCommand::~ThreadCommand(){
    if( fd ){
        closesocket( fd );
        fd = 0;
    }
    if( isSsl && ssl ){
        SSL_shutdown ( ssl );
        SSL_free ( ssl );
        isSsl = false;
        isSslAccepted = false;
        ssl = 0;
    }
    port = 0;
    ipAddress = "";
    isIpv6 = false;
}

ThreadPool::ThreadPool(int n ){
    nThreads = n;
    threadmutexes = new mutex[n];
    threadQueue   = new ThreadQueue[n];
    threadId = 0;
    int i    = 0;
    i = 0;
    while( i < MAXTHREADS ) {
        threadTaskCount[i]     = 0;
        threadTaskDoneCount[i] = 0;
        i++;
    }
    i = 0;
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
    size_t leastBusy = 10000;
    int selected = -999;
    int i = 0;
    while( i < nThreads){
        threadmutexes[i].lock();
        size_t n = threadQueue[i].size();
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

//Round robin
void ThreadPool::assignTaskRr(ThreadCommand *cmd){
    int leastBusy = 1000000000;
    int leastBusyThread = 0;
    int i = 0;

    while( i < nThreads){
        threadmutexes[i].lock();
        int n = threadTaskCount[i] - threadTaskDoneCount[i];
        if( n <= leastBusy ){
            leastBusy = n;
            leastBusyThread = i;
        }
        threadmutexes[i].unlock();
        i++;
    }

    if( leastBusyThread < nThreads){
        threadmutexes[leastBusyThread].lock();
        httpdlog("INFO", std::to_string(leastBusyThread) + ": Thread assigned the task" );
        threadQueue[leastBusyThread].push(cmd);
        threadTaskCount[leastBusyThread]++;
        threadmutexes[leastBusyThread].unlock();
    }

}

void ThreadPool::taskDone(int i){
    threadmutexes[i].lock();
    threadTaskDoneCount[i]++;
    threadmutexes[i].unlock();
}



size_t ThreadPool::isFullHeaderPresent( char *data, size_t len ){
    size_t i = 0;
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

        if(!cmd){
            std::this_thread::sleep_for(std::chrono::microseconds(10000) );
            continue;
        }

        if( cmd && cmd->isSsl ){
            httpdlog("INFO", std::to_string(id ) + ": SSL connection command request" );
        } else if ( cmd ){
            httpdlog("INFO", std::to_string(id ) + ": NONSSL connection command request" );
        }


        if( cmd == 0 || cmd->task == STBY ){
            std::this_thread::sleep_for(std::chrono::microseconds(1000) );
            if( standby ){
                httpdlog("INFO", std::to_string(id ) + ": Standing By" );
                standby = false;
            }
            continue;

        } else if ( cmd->task == STOP ) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000) );
            httpdlog("INFO", std::to_string(id ) + ": Exiting" );
            break;
        } else if ( cmd->task == WORK ) { //WORK
            std::this_thread::sleep_for(std::chrono::milliseconds(1) );
            httpdlog("INFO", std::to_string(id ) + ": Working " + (cmd->isSsl?"SSL":"Non-SSL") + (cmd->isIpv6?" - IPv6":" - IPv4")  );
            if( cmd->isSsl ){
                if(!cmd->isSslAccepted)
                {
                    httpdlog("INFO",std::to_string(id ) + ": SSL connection not accepted" + (cmd->isIpv6?" - IPv6 ":" - IPv4 ") + std::to_string( (unsigned long long int)cmd->ssl));
                    delete cmd;
                    cmd = 0;
                    continue;
                }
                httpdlog("INFO",std::to_string(id ) + ": SSL connection received" + (cmd->isIpv6?" - IPv6 ":" - IPv4 ") + std::to_string( (unsigned long long int)cmd->ssl));
                HttpRequest *req = new HttpRequest();
                size_t dataStartPresent = 0;
                size_t dataStart = 0;
                size_t nBytes = 0;
                req->m_Len = 0;
                int rc = 0;
                int count = 0;
                while( true ){
                    rc = SSL_read_ex ( cmd->ssl, req->m_Buffer, MAXBUFFER - req->m_Len, &nBytes );
                    //nBytes = recv ( cmd->fd, (char *)req->m_Buffer, MAXBUFFER-req->m_Len, 0 );
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
                    } else if ( nBytes <= 0 ){
                        if( rc == 0 ){
                            if (count++ > 500) {
                                httpdlog("INFO", std::to_string(id) + ": SSL socket received 0 data 500 times, breaking out of loop");
                                delete cmd;
                                cmd = 0;
                                break;
                            }
                            httpdlog("INFO", std::to_string(id ) + ": SSL socket received 0 Bytes : " + std::to_string(nBytes) );
                            std::this_thread::sleep_for(std::chrono::microseconds(10) );
                        } else {
                            httpdlog("INFO", std::to_string(id ) + ": Received negative bytes : " + std::to_string(nBytes) );
                            delete cmd;
                            cmd =0;
                            break;
                        }
                    }
                }

                if (!cmd) {
                    delete req;
                    req = 0;
                    continue;
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

                    HttpResponse* resp = HttpResponse::CreateFileResponse(req->m_RequestFile);
                    if (resp->m_Fhandle.is_open()) {

                        resp->BuildResponseHeader(0);
                        size_t nHttpDataBytes = resp->m_ResponseHeaderLen + resp->getFileSize();
                        httpdlog("INFO", std::to_string(id) + ": Built response header for file access, bytes to send : " + to_string(nHttpDataBytes));
                        resp->m_Fhandle.seekg(0, ios::beg);

                        long long int totalBytes = 0;
                        long long int totalFileBytes = 0;
                        long long int bufferBytes = resp->m_ResponseHeaderLen;
                        long long int offset = 0;
                        long long int partial = 0;
                        long long int n = 0;

                        resp->m_Buffer[resp->m_ResponseHeaderLen] = 0;
                        httpdlog("INFO", (char*)resp->m_Buffer);
                        while (totalBytes < nHttpDataBytes) {

                            partial = 0;
                            n = 0;

                            while (partial < bufferBytes) {
                                //n = send(cmd->fd, (char*)&(resp->m_Buffer[partial]), bufferBytes - partial, 0);
                                n = SSL_write(cmd->ssl, (char*)&(resp->m_Buffer[partial]), bufferBytes - partial);
                                if (n > 0) {
                                    partial += n;
                                }
                                else if (n < 0) {
                                }
                                else {
                                }
                            }
                            httpdlog("DEBUG", "Sent Bytes :" + to_string(partial));
                            totalBytes += partial;
                            httpdlog("DEBUG ", "Total:  " + to_string(totalBytes));
                            if (resp->m_Fhandle.eof())
                                break;
                            offset = resp->m_Fhandle.tellg();
                            if (offset == -1)
                                break;
                            httpdlog("DEBUG ", "Offset: " + to_string(offset));
                            if (resp->m_Fhandle.read((char*)(resp->m_Buffer), MAXBUFFER))
                                bufferBytes = resp->m_Fhandle.tellg() - offset;
                            else
                                bufferBytes = resp->m_ActualFileSize - totalBytes + resp->m_ResponseHeaderLen;
                            httpdlog("DEBUG ", "---------------------------------------");
                        }
                        httpdlog("DEBUG ", "Total bytes sent including header: " + to_string(totalBytes));
                        delete resp;
                    } else {
                        //httpdlog("INFO",  std::to_string(id )+": " + (char *)resp->m_Buffer );
                        delete resp; resp = 0;
                        HttpResponse* resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                        httpdlog("INFO", std::to_string(id) + ": Building response header ");
                        resp->BuildResponseHeader(0);
                        httpdlog("INFO", std::to_string(id) + ": Built response header ");
                        resp->addResponseData(resp->m_HttpData);
                        httpdlog("INFO", std::to_string(id) + ": Adding data " +(char*)(resp->m_Buffer  ) );
                        size_t nHttpDataBytes = strlen((char*)resp->m_Buffer);

                        int partial = 0, n = 0;
                        do {
                            n = SSL_write(cmd->ssl, (char*)&(resp->m_Buffer[partial]), nHttpDataBytes - partial);
                            if (n > 0) {
                                partial += n;
                            }
                            else if (n == -1) {
                                break;
                            }
                            else {
                            }
                        } while (partial < nHttpDataBytes);

                        delete resp; resp = 0;
                    }
                    httpdlog("DEBUG", std::to_string(id) + ": Deleting work object " + std::to_string((unsigned long long int)cmd));
                    closesocket(cmd->fd);
                    cmd->fd = 0;
                    delete cmd;
                    cmd = 0;
                }
                if (req) {
                    delete req; req = 0;
                }
            }
            else {
                httpdlog("INFO", std::to_string(id) + ": Working " + (cmd->isSsl ? "SSL" : "Non-SSL") + (cmd->isIpv6 ? " - IPv6" : " - IPv4"));
                HttpRequest* req = new HttpRequest();
                int dataStartPresent = 0;
                int dataStart = 0;
                int nBytes = 0;
                req->m_Len = 0;
                int count = 0;
                while (true) {
                    nBytes = recv(cmd->fd, (char*)req->m_Buffer, MAXBUFFER - req->m_Len, 0);
                    if (nBytes > 0) {
                        dataStartPresent = 0;
                        dataStartPresent = ThreadPool::isFullHeaderPresent((char*)&(req->m_Buffer[req->m_Len]), nBytes);
                        req->m_Len += nBytes;
                        if (dataStartPresent == 0) {
                            dataStart += nBytes;
                        }
                        else {
                            dataStart += dataStartPresent;
                            break;
                        }
                    }
                    else if (nBytes < 0) {
                        httpdlog("INFO", std::to_string(id) + ": Received negative bytes : " + std::to_string(nBytes));
                        delete cmd;
                        cmd = 0;
                        break;
                    }
                    else {
                        if (count++ > 500) {
                            httpdlog("INFO", std::to_string(id) + ": socket received 0 data 500 times, breaking out of loop");
                            delete cmd;
                            cmd = 0;
                            break;
                        }
                        httpdlog("INFO", std::to_string(id) + ": socket received 0 Bytes : " + std::to_string(nBytes));
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }

                if (!cmd) {
                    delete req;
                    req = 0;
                    continue;
                }

                int hLen = 0;
                HttpRequest::readHttpHeader(req, (char*)(req->m_Buffer), &hLen, req->m_Len);
                httpdlog("INFO", std::to_string(id) + ": Data starts from : " + std::to_string(dataStart) + " Total Read: " + std::to_string(req->m_Len));

                bool hasDataBeenRead = false;
                if (req->m_Len > dataStart) {
                    //Write to file, possible post data or multipart post data or put data
                }
                hasDataBeenRead = true;

                if (req->m_Len == dataStart || hasDataBeenRead) {
                    //httpdlog("INFO", std::to_string(id ) + ": "+ req->m_RequestFile );
                    HttpResponse* resp = HttpResponse::CreateFileResponse(req->m_RequestFile);
                    if (resp->m_Fhandle.is_open()) {

                        resp->BuildResponseHeader(0);
                        size_t nHttpDataBytes = resp->m_ResponseHeaderLen + resp->getFileSize();
                        httpdlog("INFO", std::to_string(id) + ": Built response header for file access, bytes to send : " + to_string(nHttpDataBytes));
                        resp->m_Fhandle.seekg(0, ios::beg);

                        long long int totalBytes = 0;
                        long long int totalFileBytes = 0;
                        long long int bufferBytes = resp->m_ResponseHeaderLen;
                        long long int offset = 0;
                        long long int partial = 0;
                        long long int n = 0;

                        resp->m_Buffer[resp->m_ResponseHeaderLen] = 0;
                        httpdlog("INFO", (char*)resp->m_Buffer);
                        while (totalBytes < nHttpDataBytes) {

                            partial = 0;
                            n = 0;

                            while (partial < bufferBytes) {
                                n = send(cmd->fd, (char*)&(resp->m_Buffer[partial]), bufferBytes - partial, 0);
                                if (n > 0) {
                                    partial += n;
                                }
                                else if (n < 0) {
                                }
                                else {
                                }
                            }
                            httpdlog("DEBUG", "Sent Bytes :" + to_string(partial));
                            totalBytes += partial;
                            httpdlog("DEBUG ", "Total:  " + to_string(totalBytes));
                            if (resp->m_Fhandle.eof())
                                break;
                            offset = resp->m_Fhandle.tellg();
                            if (offset == -1)
                                break;
                            httpdlog("DEBUG ", "Offset: " + to_string(offset));
                            if (resp->m_Fhandle.read((char*)(resp->m_Buffer), MAXBUFFER))
                                bufferBytes = resp->m_Fhandle.tellg() - offset;
                            else
                                bufferBytes = resp->m_ActualFileSize - totalBytes + resp->m_ResponseHeaderLen;
                            httpdlog("DEBUG ", "---------------------------------------");
                        }
                        httpdlog("DEBUG ", "Total bytes sent including header: " + to_string(totalBytes));
                        delete resp;
                    }
                    else {

                        delete resp; resp = 0;
                        //httpdlog("INFO",  std::to_string(id )+": " + (char *)resp->m_Buffer );
                        HttpResponse* resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                        httpdlog("INFO", std::to_string(id) + ": Building response header ");
                        resp->BuildResponseHeader(0);
                        httpdlog("INFO", std::to_string(id) + ": Built response header ");
                        resp->addResponseData(resp->m_HttpData);
                        httpdlog("INFO", std::to_string(id) + ": Adding data ");
                        size_t nHttpDataBytes = strlen((char*)resp->m_Buffer);

                        int partial = 0, n = 0;
                        do {
                            n = send(cmd->fd, (char*)&(resp->m_Buffer[partial]), nHttpDataBytes - partial, 0);
                            if (n > 0) {
                                partial += n;
                            }
                            else if (n == -1) {
                                break;
                            }
                            else {
                            }
                        } while (partial < nHttpDataBytes);

                        delete resp;
                        resp = 0;
                    }

                    httpdlog("DEBUG", std::to_string(id) + ": Deleting work object " + std::to_string((unsigned long long int)cmd));
                    closesocket(cmd->fd);
                    cmd->fd = 0;
                    delete cmd;
                    cmd = 0;
                }
                if (req) {
                    delete req; req = 0;
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(1000) );
        }
    }
}
