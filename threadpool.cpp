//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#define _CRT_SECURE_NO_WARNINGS
#include <Python.h>
#include <threadpool.h>
#include <httpdlog.h>
#include <chunkedencoding.h>

#include <algorithm>
#include <fstream>
using namespace std;

mutex *ThreadPool::threadmutexes;
ThreadQueue * ThreadPool::threadQueue;
mutex  ThreadPool::tempFileMutex;
uint64_t ThreadPool::tempNum = 0;



thread_local ThreadInfo info;
thread_local int globalThreadId = -1;

CookieManager cookieManager;

ThreadCommand::ThreadCommand( ){
    task = STBY;
    fd = 0;
    isSsl=false;
    isSslAccepted=false;
    isIpv6=false;
    int port = -1;
    ipAddress ="";
    ssl = 0;
    port = 8080;
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
                httpdlog("INFO", std::to_string( globalThreadId) + ": isFullHeaderPresent: Data starts here : "+std::to_string(i+4));
                return i+4;
            }
        }
        i++;
    }
    return 0;
}

void ThreadPool::getTempFileName(string &tempFileName) {
    tempFileMutex.lock();
    uint64_t tNum = tempNum;
    tempNum++;
    tempFileMutex.unlock();
    tempFileName = "C:\\HttpdWin\\Temp\\PostFileData-" + to_string(tNum) + ".post";
    //tempFileName = "PostFileData-" + to_string(tNum) + ".post";
}


void ThreadPool::writeToTempFile(string tempFileName, ThreadCommand *cmd, HttpRequest *req, size_t dataStart, int id) {
    ofstream f(tempFileName, ios::trunc | ios::binary);
    req->m_TempPostFileName = tempFileName;
    req->m_TempPutFileName = tempFileName;
    //f.open(tempFileName, ios::out | ios::trunc | ios::binary );
    int count = 0;
    int rc = 0;
    size_t nBytes = 0;
    if (f.is_open()) {
        httpdlog("INFO", std::to_string(id) + ": Writing to temporary file: "+ tempFileName + ", initial bytes: "+ std::to_string(req->m_Len - dataStart));
        size_t totalDataRead = 0;
        if (req->m_Len > dataStart) {
            f.write( (const char *)&(req->m_Buffer[dataStart]), req->m_Len - dataStart);
            if (f.fail()) {
                httpdlog("ERROR", std::to_string(id) + ": Failed to write data which came with the header to temporary file: " + tempFileName);
            }
            totalDataRead = req->m_Len - dataStart;
        }
        
        while ( totalDataRead < req->m_cLen ) {
            //httpdlog("DEBUG", "Looping : Total Read:  " + std::to_string(totalDataRead) );
            rc = 0;
            nBytes = 0;
            if( cmd->isSsl )
                rc = SSL_read_ex(cmd->ssl, req->m_Buffer, MAXBUFFER, &nBytes);
            else
                nBytes = recv(cmd->fd, (char*)req->m_Buffer, MAXBUFFER, 0);

            if (nBytes > 0 && nBytes <= MAXBUFFER ) {
                totalDataRead += nBytes;
                count = 0;
				//httpdlog("DEBUG", "Received Bytes :" + std::to_string(nBytes) + " rc = " + std::to_string(rc)); 
                f.write((const char *)req->m_Buffer, nBytes);
                if (f.fail()) {
                    httpdlog("ERROR", std::to_string(id) + ": Failed to write to temporary file: " + tempFileName);
                    break;
				}
            }
            else if (nBytes == 0 || nBytes > MAXBUFFER ) {
                if (cmd->isSsl) {
                    if (rc == 0) {
                        if (count++ > 50) {
                            httpdlog("INFO", std::to_string(id) + ": SSL socket received  invalid amount of bytes 50 times, breaking out of loop");
                            break;
                        }
                        httpdlog("INFO", std::to_string(id) + ": SSL socket received invalid Bytes : " + std::to_string(nBytes));
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                    else {
                        httpdlog("INFO", std::to_string(id) + ": SSL socket received negative rc : " + std::to_string(nBytes)+ ",  RC = " + to_string(rc));
                        break;
                    }
                }
                else {
                    if (count++ > 50) {
                        httpdlog("INFO", std::to_string(id) + ": Normal socket received invalid amount of bytes 50 times, breaking out of loop");
                        break;
                    }
                    httpdlog("INFO", std::to_string(id) + ": Normal socket received invalid Bytes : " + std::to_string(nBytes));
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
                
            }
        }
        f.close();
    }
    else {
        httpdlog("INFO", std::to_string(id) + ": NUnable to open temporary file for writing post data :" + tempFileName );
        req->m_TempPostFileName = "";
        req->m_TempPutFileName = "";
    }
}


void ThreadPool::sendHttpHeader() {
    info.resp->m_IsChunked = true;
    int i = 0;
    //httpdlog("INFO", std::to_string(id) + ": Building response header ce : " + extension);
    info.resp->BuildResponseHeader(0);
    info.resp->m_Buffer[info.resp->m_ResponseHeaderLen] = 0;
    //httpdlog("    ", " ");
    //httpdlog("    ", "Response Header: ");
    httpdlogHdr("    ", (char*)info.resp->m_Buffer);
    //httpdlog("    ", " ");

    int partial = 0, n = 0;

    do {
        if (info.cmd->isSsl) {
            n = SSL_write(info.cmd->ssl, (char*)&(info.resp->m_Buffer[partial]), info.resp->m_ResponseHeaderLen - partial);
            if (n > 0) {
                partial += n;
            }
            else if (n == -1) {
                //break;
            }
            else {
            }
        }
        else {
            n = send(info.cmd->fd, (char*)&(info.resp->m_Buffer[partial]), info.resp->m_ResponseHeaderLen - partial, 0);
            if (n > 0) {
                partial += n;
            }
            else if (n == -1) {
                //break;
            }
            else {
            }
        }
    } while (partial < info.resp->m_ResponseHeaderLen);
}


//len should less than 65532 bytes in one go for chunked encoding
void ThreadPool::sendHttpData(char *data, size_t len) {
    const int maxceLen = 60000;
    int i = 0;
    int count = 0;
    if (len > maxceLen) {
        while (i < len) {
            ChunkedEncoding* ce = new ChunkedEncoding();
            if ( (len - i) < maxceLen )
                ce->setData((char*)&(data[i]), len - i, false);
			else
                ce->setData((char*)&(data[i]), maxceLen, false);
            int partial = 0, n = 0;
            count = 0;
            do {
                if (info.cmd->isSsl) {
                    n = SSL_write(info.cmd->ssl, (char*)&(ce->data[partial]), ce->size - partial);
                    if (n > 0) {
                        partial += n;
                        //count = 0;
                    }
                    else if (n == -1) {
                        //break;
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        count++;
                        if (count > 50000)
                            break;
                    }
                    else {
                        count = 0;
                    }
                }
                else {
                    n = send(info.cmd->fd, (char*)&(ce->data[partial]), ce->size - partial, 0);
                    if (n > 0) {
                        partial += n;
                        //count = 0;
                    }
                    else if (n == -1) {
                        //break;
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                        count++;
                        if (count > 50000)
                            break;
                    }
                    else {
                        count = 0;
                    }
                }
            } while (partial < ce->size);
            delete ce;
            i += maxceLen;
            ce = 0;
            if (count > 50000)
                break;
        }
	}
	else {
        ChunkedEncoding* ce = new ChunkedEncoding();
        ce->setData(data, len, false);
        int i = 0, partial = 0, n = 0;
        count = 0;
        do {
            if (info.cmd->isSsl) {
                n = SSL_write(info.cmd->ssl, (char*)&(ce->data[partial]), ce->size - partial);
                if (n > 0) {
                    partial += n;
                    count = 0;
                }
                else if (n == -1) {
                    //break;
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    count++;
                    if (count > 50000)
                        break;
                }
                else {
                    count = 0;
                }
            }
            else {

                n = send(info.cmd->fd, (char*)&(ce->data[partial]), ce->size - partial, 0);
                if (n > 0) {
                    partial += n;
                    count = 0;
                }
                else if (n == -1) {
                    //break;
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                    count++;
                    if (count > 50000)
                        break;
                }
                else {
                    count = 0;
                }
            }
        } while (partial < ce->size);
        delete ce;
        ce = 0;
    }
}

void ThreadPool::sendHttpDataFinal(char* data, size_t len) {
    ChunkedEncoding* ce = new ChunkedEncoding();
    ce->setData(data, len, true);
    int i = 0, partial = 0, n = 0;
    int count = 0;
    do {
        if (info.cmd->isSsl) {
            n = SSL_write(info.cmd->ssl, (char*)&(ce->data[partial]), ce->size - partial);
            if (n > 0) {
                partial += n;
                count = 0;
            }
            else if (n == -1) {
                //break;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                count++;
                if (count > 50000)
                    break;
            }
            else {
                count = 0;
            }
        }
        else {

            n = send(info.cmd->fd, (char*)&(ce->data[partial]), ce->size - partial, 0);
            if (n > 0) {
                partial += n;
                //count = 0;
            }
            else if (n == -1) {
                //break;
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                count++;
                if (count > 50000)
                    break;
            }
            else {
                count = 0;
            }
        }
    } while (partial < ce->size);
    delete ce;
    ce = 0;
}

void ThreadPool::setCookie(string name, string value, string expires, uint64_t maxAge, bool secure, bool httpOnly, std::string path, std::string domain) {
    if (info.resp && info.resp->m_CookieList)
    {
        string gname = "";
        if (info.resp->m_CookieList->size() > 0 ) {
            gname = info.resp->m_CookieList->front().m_gname;
        }
        if (gname != "") {
            Cookie c(gname, name, value, expires, maxAge, secure, httpOnly, path, domain);
            info.resp->m_CookieList->push_back(c);
            httpdlog("DEBUG", std::to_string(globalThreadId) + ": Added cookie, setting max age to :" + name +", maxage=" + std::to_string(maxAge) );
        }
    }
}

void ThreadPool::delCookie(string name){
    if (info.resp && info.resp->m_CookieList)
    {
        string gname = "";
        if (info.resp->m_CookieList->size() > 0) {
            gname = info.resp->m_CookieList->front().m_gname;
            CookieList::iterator it = info.resp->m_CookieList->begin();
            while (it != info.resp->m_CookieList->end()) {
                if (it->m_name == name)
                {
                    //info.resp->m_CookieList->erase(it);
                    httpdlog("DEBUG", std::to_string(globalThreadId) + ": Deleting cookie, setting max age to zero : " + name );
                    it->m_maxage = 0;
                    break;
                }
                it++;
            }
        }
    }
}

void ThreadPool::clearHttpSession() {
    string str = info.resp->m_CookieList->size() > 0 ? info.resp->m_CookieList->front().m_gname : "";
    cookieManager.clear(str);
    info.req->m_CookieList = 0;
    if (info.resp->m_CookieList) {
        info.resp->m_CookieList->clear();
        delete info.resp->m_CookieList;
        info.resp->m_CookieList = 0;
    }
}

void ThreadPool::simpleChunkedResponse(int id , ThreadCommand *cmd, HttpResponse *resp, const char *simplestring) {

    ChunkedEncoding* ce = new ChunkedEncoding();
    resp->m_IsChunked = true;
    int i = 0;
    //httpdlog("INFO", std::to_string(id) + ": Building response header ce : " + extension);
    resp->BuildResponseHeader(0);

    httpdlog("INFO", std::to_string(id) + ": " + (char*)(resp->m_Buffer));
    httpdlog("INFO", std::to_string(id) + ": Built response header ce ");
    resp->m_Buffer[resp->m_ResponseHeaderLen] = 0;
    //httpdlog("    ", " ");
    //httpdlog("    ", "Response Header: ");
    httpdlogHdr("    ", (char*)resp->m_Buffer);

    int partial = 0, n = 0;

    do {
        if (cmd->isSsl) {
            n = SSL_write(cmd->ssl, (char*)&(resp->m_Buffer[partial]), resp->m_ResponseHeaderLen - partial);
            if (n > 0) {
                partial += n;
            }
            else if (n == -1) {
                //break;
            }
            else {
            }
        }
        else {
            n = send(cmd->fd, (char*)&(resp->m_Buffer[partial]), resp->m_ResponseHeaderLen - partial, 0);
            if (n > 0) {
                partial += n;
            }
            else if (n == -1) {
                //break;
            }
            else {
            }
        }
    } while (partial < resp->m_ResponseHeaderLen);

    //httpdlog("INFO", std::to_string(id) + ": " + (char*)(resp->m_Buffer));

    i = 0;
    while (i <= 10000) {
        partial = 0, n = 0;

        if (i < 10000) {
            snprintf((char*)(resp->m_Buffer), MAXBUFFER, "%s - This is chunked transfer encoding number = %d  from thread = %d:", simplestring, i, id);
            const string base64 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";
            int j = i % 64;
            strcat((char*)(resp->m_Buffer), base64.substr(0, j).c_str());
            strcat((char*)(resp->m_Buffer), "\r\n");
            ce->setData((char*)(resp->m_Buffer), strlen((char*)resp->m_Buffer), false);
        }
        else {
            resp->m_Buffer[0] = 0;
            ce->setData((char*)(resp->m_Buffer), 0, true);
        }
        do {

            if (cmd->isSsl) {
                n = SSL_write(cmd->ssl, (char*)&(ce->data[partial]), ce->size - partial);
                if (n > 0) {
                    partial += n;
                }
                else if (n == -1) {
                    break;
                }
                else {
                }
            }
            else {

                n = send(cmd->fd, (char*)&(ce->data[partial]), ce->size - partial, 0);
                if (n > 0) {
                    partial += n;
                }
                else if (n == -1) {
                    //break;
                }
                else {
                }
            }
        } while (partial < ce->size);
        i++;
    }
    delete ce;
    ce = 0;
    delete resp;
    resp = 0;

}

string ThreadPool::generateJsonFile() {
    ofstream f;
    string sessionid = info.resp->m_CookieList->size() > 0 ? info.resp->m_CookieList->front().m_gname : "";
    string jsonfile = "C:\\HttpdWin\\Temp\\_____" + sessionid.substr(0,32) +"_____" + ".json";
    httpdlog("DEBUG", std::to_string(globalThreadId) + ": Opening input file for script " + jsonfile + ", " + sessionid);
    f.open(jsonfile, ios::out | ios::trunc);
    if (f.is_open()) {
        f << "{ \n\t\"headers\" : { "<<endl;
        int i = 0;
        int n = info.req->m_Headers->size();
        while (i < n ) {
            if( i == n-1 )
                f << "\t\t\"" << info.req->m_HeaderNames[i] << "\" : \"" << info.req->m_Headers[i] << "\"" << endl;
            else
                f << "\t\t\"" << info.req->m_HeaderNames[i] << "\" : \"" << info.req->m_Headers[i] << "\", " << endl;
            i++;
        }
        f << "\t}," << endl << "\n\t\"cookies\" : {"<<endl;

        i = 0;
        n = info.resp->m_CookieList->size();
        CookieList::iterator it  = info.resp->m_CookieList->begin();
        while (it != info.resp->m_CookieList->end() ) {
            if (i == n - 1)
                f << "\t\t\"" << it->m_name << "\" : \"" << it->m_value << "\"" << endl;
            else
                f << "\t\t\"" << it->m_name << "\" : \"" << it->m_value << "\", " << endl;
            it++;
            i++;
        }
        f << "\t}," << endl << "\n\t\"url\" : \"" <<info.req->m_EncodedUrl<<"\"," << endl;
        std::replace(jsonfile.begin(), jsonfile.end(), '\\', '/');
        f << "\t\"jsonfile\" : \"" << jsonfile << "\"," << endl;
        f << "\t\"method\" : \"" << info.req->m_Method << "\"," << endl;
        f << "\t\"version\" : \"" << info.req->m_Version << "\"," << endl;
        std::replace(info.req->m_TempPostFileName.begin(), info.req->m_TempPostFileName.end(), '\\', '/');
        f << "\t\"postfile\" : \"" << info.req->m_TempPostFileName << "\"," << endl;
        std::replace(info.req->m_TempPutFileName.begin(), info.req->m_TempPutFileName.end(), '\\', '/');
        f << "\t\"putfile\" : \"" << info.req->m_TempPutFileName << "\"," << endl;
        std::replace(info.req->m_RequestFile.begin(), info.req->m_RequestFile.end(), '\\', '/');
        f << "\t\"requestfile\" : \"" << info.req->m_RequestFile << "\"," << endl;
        f << "\t\"length\" : \"" << info.req->m_Len << "\"" << endl;
        f << "}" << endl;
        f.flush();
        f.close();
    }
    else {
        httpdlog("ERROR", std::to_string(globalThreadId) + ": Unable to open file to write json content to : " + jsonfile);
    }
    return jsonfile;
}

void ThreadPool::addHttpHeader(string value) {
    info.resp->m_CollatedHeaders += value + "\r\n";
}



void ThreadPool::threadpoolFunction(int id ){
    bool standby = true;
    bool dataNotReceived = true;
    httpdlog("INFO", std::to_string(id ) + ": Thread starting up" );
    globalThreadId = id;
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
            httpdlog("WARN", std::to_string(id ) + ": Thread pool thread handling work "+ std::to_string((unsigned long long int ) cmd)+ " -> " + (cmd->isSsl ? "SSL" : "Non-SSL") + (cmd->isIpv6 ? " - IPv6" : " - IPv4"));
            
            if( cmd->isSsl ){
				//SSL Request/Read Part
                if(!cmd->isSslAccepted)
                {
                    httpdlog("ERROR",std::to_string(id ) + ": SSL connection not accepted" + (cmd->isIpv6?" - IPv6 ":" - IPv4 ") + std::to_string( (unsigned long long int)cmd->ssl));
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
                    if( nBytes > 0 && nBytes <= MAXBUFFER - req->m_Len){
                        count = 0;
                        dataStartPresent   = 0;
                        dataStartPresent   = ThreadPool::isFullHeaderPresent((char*)&(req->m_Buffer[req->m_Len]), nBytes );
                        req->m_Len += nBytes;
                        if( dataStartPresent == 0 ){
                            dataStart += nBytes;
                        } else {
                            dataStart += dataStartPresent;
                            break;
                        }
                    } else if ( nBytes == 0 || nBytes > MAXBUFFER ){
                        if( rc == 0 ){
                            if (count++ > 50) {
                                httpdlog("ERROR", std::to_string(id) + ": SSL socket received 0 data 50 times, breaking out of loop");
                                delete cmd;
                                cmd = 0;
                                break;
                            }
                            httpdlog("ERROR", std::to_string(id ) + ": SSL socket received 0 Bytes : " + std::to_string(nBytes) );
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
                    httpdlog("ERROR", std::to_string(id) + ": Socket Error, Not handling request ");
                    delete req;
                    req = 0;
                    httpdlog("ERROR", std::to_string(id) + ": Assigned thread completed with error, rejoining threadpool");
                    continue;
                }

                int hLen = 0;
                //httpdlog("    ", "==============================================================");
                HttpRequest::readHttpHeader ( req, (char *)(req->m_Buffer), &hLen, req->m_Len );
                httpdlog("INFO", std::to_string(id ) + ": Data starts from : " + std::to_string(dataStart) + " Total Read: " + std::to_string(req->m_Len) );
                
                bool hasDataBeenRead = false;
                if( req->m_Len > dataStart || req->m_cLen > 0 ){
                    //Write to file, possible post data or multipart post data or put data
                    httpdlog("INFO", "Post or Put Data present ");
                    string tfn;
                    getTempFileName(tfn);
                    writeToTempFile(tfn, cmd, req, dataStart, id);
                }
                hasDataBeenRead = true;

                //SSL Response Part

                if( req->m_Len == dataStart || hasDataBeenRead ){

                    HttpResponse* resp = HttpResponse::CreateFileResponse(req->m_RequestFile);
					resp->m_DecodedUrl = req->m_DecodedUrl;
                    string extension = resp->m_Extension;
                    if ( ( extension != "" && extension != "script") && resp->m_Fhandle.is_open() ) {
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
                        //httpdlog("    ", " ");
                        //httpdlog("    ", "Response Header: ");
                        httpdlogHdr("    ", (char*)resp->m_Buffer);
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
                            //httpdlog("DEBUG", "Sent Bytes :" + to_string(partial));
                            totalBytes += partial;
                            //httpdlog("DEBUG ", "Total:  " + to_string(totalBytes));
                            if (resp->m_Fhandle.eof())
                                break;
                            offset = resp->m_Fhandle.tellg();
                            if (offset == -1)
                                break;
                            //httpdlog("DEBUG ", "Offset: " + to_string(offset));
                            if (resp->m_Fhandle.read((char*)(resp->m_Buffer), MAXBUFFER))
                                bufferBytes = resp->m_Fhandle.tellg() - offset;
                            else
                                bufferBytes = resp->m_ActualFileSize - totalBytes + resp->m_ResponseHeaderLen;
                            //httpdlog("DEBUG ", "---------------------------------------");
                        }
                        httpdlog("DEBUG ", std::to_string(id) + ": Total bytes sent including header: " + to_string(totalBytes));
                        delete resp;
                    } else {
						//This is where code for scripting languages like PHP, Python, etc. will come in
                        delete resp; resp = 0;

                        HttpResponse* resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                        resp->m_DecodedUrl = req->m_DecodedUrl;
                        if (extension == "script" || extension == "" ) {
                            Cookie* c = 0;
                            if (req->m_CookieList == 0 || (req->m_CookieList != 0 && req->m_CookieList->size() == 0)) {
                                c = new Cookie("", "", "");
                                CookieList* cl = new CookieList();
                                bool status = false;

                                do {
                                    c->generateSessionId();
                                    //httpdlog("DEBUG", "Cookie not present, creating new : " + c->m_gname + ", " + c->m_value);
                                    if (resp->m_CookieList == 0)
                                        resp->m_CookieList = new CookieList();
                                    resp->m_CookieList->push_back(*c);
                                    
                                    for (auto it1 = resp->m_CookieList->begin(); it1 != resp->m_CookieList->end(); it1++) {
                                        cl->push_back(*it1);
                                    }
                                    status = cookieManager.add(c->m_gname, cl);
                                    cookieManager.print();
                                    if( status == false) {
                                        //httpdlog("DEBUG", "Cookie present for : " + c->m_gname +", " + c->m_value);
                                        resp->m_CookieList->clear();
                                        cl->clear();
									}
							    } while (status == false);  
                            }
                            else {
                                if (resp->m_CookieList == 0)
                                    resp->m_CookieList = new CookieList();
								auto it = req->m_CookieList->begin();
                                if (it != req->m_CookieList->end()) {
                                    CookieList* cl = 0;
                                    cl = cookieManager.get((*it).m_gname);
                                    cookieManager.print();

                                    if (cl != 0) {
                                        httpdlog("DEBUG", std::to_string(id) + ": CookieMap was present for : " + (*it).m_gname);
                                        for (auto it1 = cl->begin(); it1 != cl->end(); it1++) {
                                            resp->m_CookieList->push_back(*it1);
                                        }
                                    }
                                    else {
                                        httpdlog("DEBUG", std::to_string(id) + ": CookieMap was not present for : " + (*it).m_gname +", " + (*it).m_value);
                                        Cookie* c = new Cookie("", "", "");
                                        cl = new CookieList();
                                        bool status = false;

                                        do {
                                            c->generateSessionId();
                                            //httpdlog("DEBUG", "Cookie not present, creating new : " + c->m_gname +", "+c->m_value);
                                            if (resp->m_CookieList == 0)
                                                resp->m_CookieList = new CookieList();
                                            resp->m_CookieList->push_back(*c);

                                            for (auto it1 = resp->m_CookieList->begin(); it1 != resp->m_CookieList->end(); it1++) {
                                                cl->push_back(*it1);
                                            }
                                            status = cookieManager.add(c->m_gname, cl);
                                            //cookieManager.print();

                                            if (status == false) {
                                                //httpdlog("DEBUG", "Cookie present for : " + c->m_gname +", " + c->m_value);
                                                resp->m_CookieList->clear();
                                                cl->clear();
                                            }
                                        } while (status == false);
                                    }
                                }
                                else {
									httpdlog("ERROR", std::to_string(id) + ": Cookie list iterator error");
                                }
                            }
#if 0
                            simpleChunkedRespone(id, cmd, resp, " From SSL response sourcecode ");
#endif

                            PyGILState_STATE gstate = PyGILState_Ensure();

                            PyThreadState* mainstate = PyThreadState_Get();
                            PyInterpreterState* maininter = PyThreadState_GetInterpreter(mainstate);
                            PyThreadState_Swap(NULL);
                            PyThreadState* tState = Py_NewInterpreter();
                            PyInterpreterState* interpreter = tState->interp;
                            PyThreadState_Swap(tState);



                            try {
                                //PyRun_SimpleString("print('Hello from thread!')");
                                string scriptFile = "C:\\HttpdWin\\Pages\\"+req->m_RequestFile.substr(1, req->m_RequestFile.length());
                                httpdlog("WARN", std::to_string(id) + ": Executing script from location : " + scriptFile );
                                info.req  = req;
                                info.resp = resp;
                                info.cmd  = cmd;

                                FILE* fp = fopen(scriptFile.c_str(), "r");
                                if (fp) {
                                    PyObject* main_module = PyImport_AddModule("__main__");
                                    PyObject* main_dict = PyModule_GetDict(main_module);
                                    req->m_jsonfile = resp->m_CookieList->size() > 0 ? resp->m_CookieList->front().m_gname : "";
                                    string jsonfile = generateJsonFile();
                                    PyObject* py_value = PyUnicode_FromString(req->m_jsonfile.c_str());
                                    PyObject* py_value2 = PyUnicode_FromString(jsonfile.c_str());
                                    PyDict_SetItemString(main_dict, "sessionid", py_value);
                                    PyDict_SetItemString(main_dict, "input", py_value2);

                                    PyRun_SimpleFile(fp, scriptFile.c_str());

                                    Py_DECREF(py_value);
                                    Py_DECREF(py_value2);
                                }
                                if (fp)
                                    fclose(fp);

                                info.req  = 0;
                                info.resp = 0;
                                info.cmd  = 0;
                            }
                            catch (exception msg) {

                            }
                            Py_EndInterpreter(tState);
                            PyThreadState_Swap(mainstate);
                            PyGILState_Release(gstate);
                            delete resp;
                            resp = 0;
                        }
                        else {
                            //httpdlog("INFO",  std::to_string(id )+": " + (char *)resp->m_Buffer );

                            HttpResponse* resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                            resp->m_DecodedUrl = req->m_DecodedUrl;
                            httpdlog("INFO", std::to_string(id) + ": Building response header ");
                            resp->m_StatusCode = "404";
                            resp->m_StatusMessage = "File not found";
                            resp->BuildResponseHeader(0);
                            httpdlog("INFO", std::to_string(id) + ": Built response header ");
                            string responseData = "404 - File not found";
                            resp->addResponseData(responseData);
                            //resp->addResponseData(resp->m_HttpData);
                            //httpdlog("INFO", std::to_string(id) + ": Adding data " + (char*)(resp->m_Buffer));
                            size_t nHttpDataBytes = strlen((char*)resp->m_Buffer);
                            //httpdlog("    ", " ");
                            //httpdlog("    ", "Response Header: ");
                            //httpdlog("    ", (char*)resp->m_Buffer);
                            httpdlogHdr("    ", (char*)resp->m_Buffer);
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
                        }

                        delete resp; resp = 0;
                    }
                    httpdlog("WARN", std::to_string(id) + ": Deleting work object " + std::to_string((unsigned long long int)cmd));
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
                HttpRequest* req = new HttpRequest();
                int dataStartPresent = 0;
                int dataStart = 0;
                int nBytes = 0;
                req->m_Len = 0;
                int count = 0;
                while (true) {
                    nBytes = recv(cmd->fd, (char*)req->m_Buffer, MAXBUFFER - req->m_Len, 0);
                    if (nBytes > 0 && nBytes <= MAXBUFFER - req->m_Len ) {
                        count = 0;
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
                        httpdlog("ERROR", std::to_string(id) + ": Received negative bytes : " + std::to_string(nBytes));
                        delete cmd;
                        cmd = 0;
                        break;
                    }
                    else {
                        if (count++ > 50) {
                            httpdlog("ERROR", std::to_string(id) + ": socket received 0 data 50 times, breaking out of loop");
                            delete cmd;
                            cmd = 0;
                            break;
                        }
                        httpdlog("XTRA", std::to_string(id) + ": socket received 0 Bytes : " + std::to_string(nBytes));
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }

                if (!cmd) {
                    httpdlog("ERROR", std::to_string(id) + ": Socket Error, Not handling request ");
                    delete req;
                    req = 0;
                    httpdlog("ERROR", std::to_string(id) + ": Assigned thread completed with error, rejoining threadpool");
                    continue;
                }

                int hLen = 0;
                //httpdlog("    ", "==============================================================");
                HttpRequest::readHttpHeader(req, (char*)(req->m_Buffer), &hLen, req->m_Len);
                httpdlog("INFO", std::to_string(id) + ": Data starts from : " + std::to_string(dataStart) + " Total Read: " + std::to_string(req->m_Len));
                bool hasDataBeenRead = false;

                if (req->m_Len > dataStart || req->m_cLen > 0) {
                    //Write to file, possible post data or multipart post data or put data
                    httpdlog("INFO", std::to_string(id) + ": Post or Put Data present ");
                    string tfn;
                    getTempFileName(tfn);
                    writeToTempFile(tfn, cmd, req, dataStart, id);
                }

                hasDataBeenRead = true;


				///Non SSL Response Part

                if (req->m_Len == dataStart || hasDataBeenRead) {
                    //httpdlog("INFO", std::to_string(id ) + ": "+ req->m_RequestFile );
                    HttpResponse* resp = HttpResponse::CreateFileResponse(req->m_RequestFile);
                    resp->m_DecodedUrl = req->m_DecodedUrl;
                    string extension = resp->m_Extension;
                    if ( (extension != "" && extension != "script") && resp->m_Fhandle.is_open() ) {

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
                        //httpdlog("    ", " ");
                        //httpdlog("    ", "Response Header: ");
                        httpdlogHdr("    ", (char*)resp->m_Buffer);
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
                            //httpdlog("DEBUG", "Sent Bytes :" + to_string(partial));
                            totalBytes += partial;
                            //httpdlog("DEBUG ", "Total:  " + to_string(totalBytes));
                            if (resp->m_Fhandle.eof())
                                break;
                            offset = resp->m_Fhandle.tellg();
                            if (offset == -1)
                                break;
                            //httpdlog("DEBUG ", "Offset: " + to_string(offset));
                            if (resp->m_Fhandle.read((char*)(resp->m_Buffer), MAXBUFFER))
                                bufferBytes = resp->m_Fhandle.tellg() - offset;
                            else
                                bufferBytes = resp->m_ActualFileSize - totalBytes + resp->m_ResponseHeaderLen;
                            //httpdlog("DEBUG ", "---------------------------------------");
                        }
                        httpdlog("DEBUG ", std::to_string(id) + ": Total bytes sent including header: " + to_string(totalBytes));
                        delete resp;
                    }
                    else {
                        //This is where code for scripting languages like PHP, Python, etc. will come in
                        delete resp; resp = 0;

                        HttpResponse* resp = HttpResponse::CreateSimpleResponse(req->m_RequestFile);
                        resp->m_DecodedUrl = req->m_DecodedUrl;
                        if (extension == "script" ||  extension == "") {


                            Cookie* c = 0;
                            if (req->m_CookieList == 0 || (req->m_CookieList != 0 && req->m_CookieList->size() == 0)) {
                                c = new Cookie("", "", "");
                                CookieList* cl = new CookieList();
                                bool status = false;

                                do {
                                    c->generateSessionId();
                                    //httpdlog("DEBUG", "Cookie not present, creating new : " + c->m_gname + ", " + c->m_value);
                                    if (resp->m_CookieList == 0)
                                        resp->m_CookieList = new CookieList();
                                    resp->m_CookieList->push_back(*c);

                                    for (auto it1 = resp->m_CookieList->begin(); it1 != resp->m_CookieList->end(); it1++) {
                                        cl->push_back(*it1);
                                    }
                                    status = cookieManager.add(c->m_gname, cl);
                                    cookieManager.print();
                                    if (status == false) {
                                        //httpdlog("DEBUG", "Cookie present for : " + c->m_gname +", " + c->m_value);
                                        resp->m_CookieList->clear();
                                        cl->clear();
                                    }
                                } while (status == false);
                            }
                            else {
                                if (resp->m_CookieList == 0)
                                    resp->m_CookieList = new CookieList();
                                auto it = req->m_CookieList->begin();
                                if (it != req->m_CookieList->end()) {
                                    CookieList* cl = 0;
                                    cl = cookieManager.get((*it).m_gname);
                                    cookieManager.print();

                                    if (cl != 0) {
                                        httpdlog("DEBUG", std::to_string(id) + ": CookieMap was present for : " + (*it).m_gname);
                                        for (auto it1 = cl->begin(); it1 != cl->end(); it1++) {
                                            resp->m_CookieList->push_back(*it1);
                                        }
                                    }
                                    else {
                                        httpdlog("DEBUG", std::to_string(id) + ": CookieMap was not present for : " + (*it).m_gname + ", " + (*it).m_value);
                                        Cookie* c = new Cookie("", "", "");
                                        cl = new CookieList();
                                        bool status = false;

                                        do {
                                            c->generateSessionId();
                                            //httpdlog("DEBUG", "Cookie not present, creating new : " + c->m_gname +", "+c->m_value);
                                            if (resp->m_CookieList == 0)
                                                resp->m_CookieList = new CookieList();
                                            resp->m_CookieList->push_back(*c);

                                            for (auto it1 = resp->m_CookieList->begin(); it1 != resp->m_CookieList->end(); it1++) {
                                                cl->push_back(*it1);
                                            }
                                            status = cookieManager.add(c->m_gname, cl);
                                            //cookieManager.print();

                                            if (status == false) {
                                                //httpdlog("DEBUG", "Cookie present for : " + c->m_gname +", " + c->m_value);
                                                resp->m_CookieList->clear();
                                                cl->clear();
                                            }
                                        } while (status == false);
                                    }
                                }
                                else {
                                    httpdlog("ERROR", std::to_string(id) + "Cookie list iterator error");
                                }
                            }
#if 0
                            simpleChunkedRespone(id, cmd, resp, " From Non-SSL response code ");
#endif

                            PyGILState_STATE gstate = PyGILState_Ensure();

                            PyThreadState* mainstate = PyThreadState_Get();
                            PyInterpreterState* maininter = PyThreadState_GetInterpreter(mainstate);
                            PyThreadState_Swap(NULL);
                            PyThreadState* tState = Py_NewInterpreter();
                            PyInterpreterState* interpreter = tState->interp;
                            PyThreadState_Swap(tState);

                            try {
                                //PyRun_SimpleString("print('Hello from thread!')");
                                string scriptFile = "C:\\HttpdWin\\Pages\\" + req->m_RequestFile.substr(1, req->m_RequestFile.length());
                                httpdlog("WARN", std::to_string(id) + ": Executing script from location : " + scriptFile);
                                info.req = req;
                                info.resp = resp;
                                info.cmd = cmd;

                                FILE* fp = fopen(scriptFile.c_str(), "r");
                                if (fp) {
                                    PyObject* main_module = PyImport_AddModule("__main__");
                                    PyObject* main_dict = PyModule_GetDict(main_module);
                                    req->m_jsonfile = resp->m_CookieList->size() > 0 ? resp->m_CookieList->front().m_gname : "";
                                    string jsonfile = generateJsonFile();
                                    PyObject* py_value  = PyUnicode_FromString(req->m_jsonfile.c_str());
                                    PyObject* py_value2 = PyUnicode_FromString(jsonfile.c_str());
                                    PyDict_SetItemString(main_dict, "sessionid", py_value);
                                    PyDict_SetItemString(main_dict, "input", py_value2);

                                    PyRun_SimpleFile(fp, scriptFile.c_str());

                                    Py_DECREF(py_value);
                                    Py_DECREF(py_value2);
                                }
                                if (fp)
                                    fclose(fp);

                                info.req = 0;
                                info.resp = 0;
                                info.cmd = 0;
                            }
                            catch (exception msg) {

                            }

                            Py_EndInterpreter(tState);
                            PyThreadState_Swap(mainstate);

                            PyGILState_Release(gstate);
                            delete resp;
                            resp = 0;

                            
                        }
                        else {

                            httpdlog("INFO", std::to_string(id) + ": Building response header ");
                            resp->m_StatusCode = "404";
                            resp->m_StatusMessage = "File not found";
                            resp->BuildResponseHeader(0);
                            httpdlog("INFO", std::to_string(id) + ": Built response header ");
                            string responseData = "404 - File not found";
                            resp->addResponseData(responseData);
                            //resp->addResponseData(resp->m_HttpData);
                            //httpdlog("INFO", std::to_string(id) + ": Adding data ");
                            size_t nHttpDataBytes = strlen((char*)resp->m_Buffer);

                            int partial = 0, n = 0;
                            do {
                                n = send(cmd->fd, (char*)&(resp->m_Buffer[partial]), nHttpDataBytes - partial, 0);
                                if (n > 0) {
                                    partial += n;
                                }
                                else if (n == -1) {
                                    //break;
                                }
                                else {
                                }
                            } while (partial < nHttpDataBytes);

                            resp->m_Buffer[resp->m_ResponseHeaderLen] = 0;
                            //httpdlog("    ", " ");
                            //httpdlog("    ", "Response Header: ");
                            httpdlogHdr("    ", (char*)resp->m_Buffer);
                        }

                        delete resp;
                        resp = 0;
                    }

                    httpdlog("WARN", std::to_string(id) + ": Deleting work object " + std::to_string((unsigned long long int)cmd));
                    
                    closesocket(cmd->fd);
                    cmd->fd = 0;
                    delete cmd;
                    cmd = 0;
                }
                if (req) {
                    delete req; req = 0;
                }
            }
            httpdlog("WARN", std::to_string(id) + ": Assigned thread completed job, rejoining threadpool");
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(1000) );
        }
    }
    
}
