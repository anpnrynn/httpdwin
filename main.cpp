//Copyright Anoop Kumar Narayanan - 2025 httpdwin

#include <Python.h>

#include <iostream>
#include <fstream>
#include <atomic>
#include <map>
#include <exception>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <malloc.h>

#include <thread>
#include <chrono>


#include <openssl/ssl.h>
#include <openssl/err.h>

#include <version.h>
#include <httpdlog.h>
#include <threadpool.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#pragma comment(lib, "python313.lib")

using namespace std;

typedef map<string, string> Config;

Config httpdwinConfig;


int parseConfig(){
    fstream f;
    f.open("C:\\Httpdwin\\httpdwin.conf", ios::in);
    if( f.is_open() ){
        string line;
         while( !f.eof() ){
            std::getline( f, line);
            if (line[0] == '#')
                continue;
            if (line.length() <= 2)
                break;
            size_t m = line.find('=', 1 );
            string name  = line.substr(0,m );
            string value = line.substr(m+1, line.length()-m-1);
            if( value[value.length()-1] == '\r' )
                value.pop_back();
            if( name != "" && value != "" ){
                httpdwinConfig[name ]= value;
                httpdlog("INFO", name +" = "+ value + " value read." );
            }
        }
    } else {
        return 1;
    }
    f.close();
    return 0;
}

extern thread_local ThreadInfo info;

static PyObject* wwwprint(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwprint called ");
    char* strval = 0;
    Py_ssize_t  strlength = 0;
    if (!PyArg_ParseTuple(args, "s#", &strval, &strlength)) {
        return NULL;  // Error: wrong arguments
    }
    if (strval && strlength > 0)
        ThreadPool::sendHttpData(strval, strlength);
    Py_RETURN_NONE;
}

static PyObject* wwwbytesprint(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwbyteprint called ");
    char* bytesval = 0;
    Py_ssize_t  byteslength = 0;
    if (!PyArg_ParseTuple(args, "y#", &bytesval, &byteslength)) {
        return NULL;  // Error: wrong arguments
    }
    if (bytesval && byteslength > 0)
        ThreadPool::sendHttpData(bytesval, byteslength);
    Py_RETURN_NONE;
}

static PyObject* wwwprintend(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwprintend called ");
    char* strval  = 0;
    Py_ssize_t  strlength = 0;
    if (!PyArg_ParseTuple(args, "s#", &strval, &strlength)) {
        return NULL;  // Error: wrong arguments
    }
    if (strval)
        ThreadPool::sendHttpDataFinal(strval, strlength);
    Py_RETURN_NONE;
}

static PyObject* wwwheadercomplete(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwheadercomplete called ");
    ThreadPool::sendHttpHeader();
    Py_RETURN_NONE;
}

static PyObject* wwwheaderadd(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwheaderadd called ");
    char* strval = 0;
    Py_ssize_t  strlength = 0;
    if (!PyArg_ParseTuple(args, "s#", &strval, &strlength)) {
        return NULL;  // Error: wrong arguments
    }
    if (strval && strlength > 0)
        ThreadPool::addHttpHeader(strval);
    Py_RETURN_NONE;
}

static PyObject* wwwmime(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwmime called ");
    char* strval = 0;
    Py_ssize_t strlength = 0;
    if (!PyArg_ParseTuple(args, "s#", &strval, &strlength)) {
        return NULL;  // Error: wrong arguments
    }
    if (strval && strlength > 0)
        info.resp->m_ContentType = strval;
    else
        info.resp->m_ContentType = "text/plain";
    Py_RETURN_NONE;
}

static PyObject* wwwsessionclear(PyObject* self, PyObject* args) {
    httpdlog("DEBUG", "wwwsessionclear called ");
    ThreadPool::clearHttpSession();
    Py_RETURN_NONE;
}

static PyMethodDef HttpdWinMethods[] = {
    {"wwwprint", wwwprint, METH_VARARGS, "Prints onto the www browser"},
    {"wwwbytesprint", wwwbytesprint, METH_VARARGS, "Prints byte data onto the www browser"},
    {"wwwprintend", wwwprintend, METH_VARARGS, "Prints the final content onto the www browser"},
    {"wwwheadercomplete", wwwheadercomplete, METH_VARARGS, "Sends the header"},
    {"wwwmime", wwwmime, METH_VARARGS, "Sets Content-Type field" },
    {"wwwheaderadd", wwwheaderadd, METH_VARARGS, "Sets headers, Omit the carriage return and linefeed" },
    {"wwwsessionclear", wwwsessionclear, METH_VARARGS, "Clears all cookies of a session"},
    {NULL, NULL, 0, NULL}  // Sentinel
};

struct PyModuleDef HttpdWin = {
    PyModuleDef_HEAD_INIT,
    "HttpdWin",     // Module name
    "HttpdWin support module",           // Optional docstring
    -1,             // Size of per-interpreter state
    HttpdWinMethods
};

PyMODINIT_FUNC PyInit_HttpdWin(void) {
    return PyModule_Create(&HttpdWin);
}


int main()
{
    httpdlog(" ","Starting up...");

    parseConfig();

    httpdlog(" ", "Configuration read");
    SOCKET srv = 0, client = 0;
    SOCKET srv6 = 0, client6 = 0;
    SOCKET sslsrv =0, sslclient = 0;
    SOCKET sslsrv6 = 0, sslclient6 = 0;

    extern struct PyModuleDef HttpdWin;

    PyImport_AppendInittab("HttpdWin", PyInit_HttpdWin);
    Py_Initialize();
    PyImport_ImportModule("HttpdWin");
    Py_BEGIN_ALLOW_THREADS

    short int serverPort  = atoi(httpdwinConfig["httpport"].c_str());
    short int serverPort6 = serverPort;
    short int sslserverPort = atoi(httpdwinConfig["httpsport"].c_str());
    short int sslserverPort6 = sslserverPort;

    uint8_t transport = 3;

    try {
        if (httpdwinConfig["transport"] == "secure") {
            transport = 2;
        }
        else if (httpdwinConfig["transport"] == "non-secure") {
            transport = 1;
        }
        else {
            transport = 3;
        }
    }
    catch (...) {
        
    }

    httpdloglevel = atoi(httpdwinConfig["debuglevel"].c_str());
    if (httpdloglevel < 0)
        httpdloglevel = 0;

    char logMsg[256];
    snprintf( logMsg, 254, "Using HTTP port = %d and, HTTPS port = %d ", serverPort, sslserverPort);

    httpdlog("INFO", logMsg);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
            httpdlog("ERROR", "WSAStartup failed\n"); return 1;
    }


    sockaddr_in  srvAddr;
    sockaddr_in  clientAddr;

    sockaddr_in6 srvAddr6;
    sockaddr_in6 clientAddr6;

    sockaddr_in  sslsrvAddr;
    sockaddr_in  sslclientAddr;

    sockaddr_in6 sslsrvAddr6;
    sockaddr_in6 sslclientAddr6;

    httpdlog (  "INFO","Creating socket(s)" );
    srv = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
    srvAddr.sin_family  = AF_INET;
    srvAddr.sin_addr.s_addr     = 0x00000000;
    srvAddr.sin_port   = htons ( serverPort );
    httpdlog (  "INFO", "IPv4 Socket created successfully" );

    srv6 = socket(PF_INET6,  SOCK_STREAM, IPPROTO_TCP);
    memset(&srvAddr6, 0, sizeof(struct sockaddr_in6));
    srvAddr6.sin6_family = AF_INET6;
    srvAddr6.sin6_addr = in6addr_loopback;
    srvAddr6.sin6_port = htons ( serverPort6 );
    httpdlog (  "INFO","IPv6 Socket created successfully" );

    httpdlog (  "INFO","Creating ssl socket(s)" );
    sslsrv = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
    sslsrvAddr.sin_family  = AF_INET;
    sslsrvAddr.sin_addr.s_addr     = 0x00000000;
    sslsrvAddr.sin_port   = htons ( sslserverPort );
    httpdlog (  "INFO","SSL IPv4 Socket created successfully" );

    sslsrv6 = socket(PF_INET6,  SOCK_STREAM, IPPROTO_TCP);
    memset(&sslsrvAddr6, 0, sizeof(struct sockaddr_in6));
    sslsrvAddr6.sin6_family = AF_INET6;
    sslsrvAddr6.sin6_addr = in6addr_loopback;
    sslsrvAddr6.sin6_port = htons ( sslserverPort6 );
    httpdlog (  "INFO", "SSL IPv6 Socket created successfully" );


    int sockflag = 1;
    int sockret = setsockopt ( srv, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        httpdlog (  "ERROR", "Unable to setsockopt - IPv4" );
        return 1;
    }

    sockflag = 1;
    sockret = setsockopt ( srv6, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        httpdlog (  "ERROR", "Unable to setsockopt - IPv6" );
        return 1;
    }

    sockflag = 1;
    sockret = setsockopt ( sslsrv, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        httpdlog (  "ERROR", "Unable to setsockopt - SSLIPv4" );
        return 1;
    }

    sockflag = 1;
    sockret = setsockopt ( sslsrv6, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        httpdlog (  "ERROR", "Unable to setsockopt - SSLIPv6" );
        return 1;
    }


    int count = 0;
    while ( bind ( srv, ( const sockaddr * ) &srvAddr, sizeof ( sockaddr_in ) ) == SOCKET_ERROR ) {
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            return 11;
        } else {
            count++;
        }
    }
    count = 0;

    while ( bind ( srv6, ( const sockaddr * ) &srvAddr6, sizeof ( sockaddr_in6 ) ) == SOCKET_ERROR ){
        char address[64];
        inet_ntop( AF_INET6, &(srvAddr6.sin6_addr), address , 64 );
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            return 12;
        } else {
            count++;
        }
    }

    count = 0;
    while ( bind ( sslsrv, ( const sockaddr * ) &sslsrvAddr, sizeof ( sockaddr_in ) ) == SOCKET_ERROR ) {
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            return 13;
        } else {
            count++;
        }
    }

    count = 0;
    while ( bind ( sslsrv6, ( const sockaddr * ) &sslsrvAddr6, sizeof ( sockaddr_in6 ) ) == SOCKET_ERROR ) {
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            return 14;
        } else {
            count++;
        }
    }

    httpdlog (  "INFO", "Bound successfully both IPv4 and IPv6" );

    if ( listen ( srv, 20 ) != 0 ) {
        httpdlog (  "ERROR", " Unable to setup backlog - IPV4" );
    }

    if ( listen ( srv6, 20 ) != 0 ) {
        httpdlog (  "ERROR", " Unable to setup backlog - IPv6" );
    }

    if ( listen ( sslsrv, 20 ) != 0 ) {
        httpdlog (  "ERROR", " Unable to setup backlog - SSL IPV4" );
    }

    if ( listen ( sslsrv6, 20 ) != 0 ) {
        httpdlog (  "ERROR", " Unable to setup backlog - SSL IPv6" );
    }

    httpdlog (  "INFO", "Done setting up Listen" );

    unsigned long mode = 1;
    ioctlsocket(srv, FIONBIO, &mode);
    ioctlsocket(srv6, FIONBIO, &mode);
    ioctlsocket(sslsrv, FIONBIO, &mode);
    ioctlsocket(sslsrv6, FIONBIO, &mode);


    const SSL_METHOD *mIp4;
    SSL_CTX *cIp4;
    mIp4 = TLS_server_method();
    cIp4 = SSL_CTX_new ( mIp4 );

    if ( !cIp4 ) {
        httpdlog (  "ERROR ", "Unable to create SSL context" );
        return ( 1000 );
    }

    if ( SSL_CTX_use_certificate_file ( cIp4, "C:\\Httpdwin\\Certs\\httpdwincert.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Certificate file issue IPv4 C:\\Httpdwin\\Certs\\httpdwincert.pem" );
        return ( 1001 );
    } else {
        httpdlog (  "INFO", "Certiticate file loaded C:\\Httpdwin\\Certs\\httpdwincert.pem" );
    }

    if ( SSL_CTX_use_PrivateKey_file ( cIp4,"C:\\Httpdwin\\Certs\\httpdwinkey.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Private key file issue IPv4 C:\\Httpdwin\\Certs\\httpdwinkey.pem" );
        return ( 1002 );
    } else {
        httpdlog (  "INFO", "Private key file loaded C:\\Httpdwin\\Certs\\httpdwinkey.pem" );
    }

    const SSL_METHOD *mIp6;
    SSL_CTX *cIp6;
    mIp6 = TLS_server_method();
    cIp6 = SSL_CTX_new ( mIp6 );

    if ( !cIp6 ) {
        httpdlog (  "ERROR", "Unable to create SSL context" );
        return ( 1000 );
    }

    if ( SSL_CTX_use_certificate_file ( cIp6, "C:\\Httpdwin\\Certs\\httpdwincert6.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Certificate file issue IPv6 C:\\Httpdwin\\Certs\\httpdwincert6.pem" );
        return ( 1001 );
    } else {
        httpdlog (  "INFO", "Certificate file loaded IPv6 C:\\Httpdwin\\Certs\\httpdwincert6.pem" );
    }

    if ( SSL_CTX_use_PrivateKey_file ( cIp6,  "C:\\Httpdwin\\Certs\\httpdwinkey6.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Private key file issue IPv6 C:\\Httpdwin\\Certs\\httpdwinkey6.pem" );
        return ( 1002 );
    } else {
        httpdlog (  "INFO", "Private key file loaded IPv6 C:\\Httpdwin\\Certs\\httpdwinkey6.pem" );
    }

    extern CookieManager cookieManager;
    cookieManager.openFileForReading();
    cookieManager.loadAll();
    cookieManager.closeFile();

	httpdlog("INFO", "Entering main loop");

    WSAPOLLFD    *pollfds = new WSAPOLLFD[5];

    int nPorts   = 4;
    int nThreads = atoi( httpdwinConfig["threads"].c_str() );

    snprintf(logMsg,254, "Starting %d Threads ", nThreads);
    httpdlog("INFO", logMsg);

    ThreadPool *tp = new ThreadPool(nThreads);
    int saveCount = 0;
    while ( true ) {
        fflush ( stderr );

        pollfds[0].fd = srv;
        pollfds[0].events = POLLIN;
        pollfds[0].revents = 0;

        pollfds[1].fd = srv6;
        pollfds[1].events = POLLIN;
        pollfds[1].revents = 0;

        pollfds[2].fd = sslsrv;
        pollfds[2].events = POLLIN;
        pollfds[2].revents = 0;

        pollfds[3].fd = sslsrv6;
        pollfds[3].events = POLLIN;
        pollfds[3].revents = 0;

        int rc = 0;
        int q = 0;
        if ((rc = WSAPoll(pollfds, nPorts, 1)) != SOCKET_ERROR) {
            q++;
            if (q % 10000 == 0)
                httpdlog("Debug", "Looping in poll");

            if (pollfds[0].revents & POLLIN && transport & 0x01 ) {
                socklen_t addrlen = sizeof(sockaddr_in);
                if ((client = accept(srv, (sockaddr*)&clientAddr, &addrlen))) {
                    httpdlog("INFO", "Assigning IPv4 task ");
                    tp->assignTaskRr(new ThreadCommand(WORK, client, false, false, false, 0, "", 0));
                }
            }

            if (pollfds[1].revents & POLLIN && transport & 0x01 ) {
                socklen_t addrlen = sizeof(sockaddr_in6);
                if ((client6 = accept(srv6, (sockaddr*)&clientAddr6, &addrlen))) {
                    httpdlog("INFO", "Assigning IPv6 task ");
                    tp->assignTaskRr(new ThreadCommand(WORK, client6, false, false, true, 0, "", 0));
                }
            }

            if (pollfds[2].revents & POLLIN && transport & 0x02) {
                socklen_t addrlen = sizeof(sockaddr_in);
                if ((sslclient = accept(sslsrv, (sockaddr*)&sslclientAddr, &addrlen))) {
                    SSL* ssl = SSL_new(cIp4);
                    if (!ssl) {
                        httpdlog("INFO", "SSL ipv4 object creation failed");
                        continue;
                    }
                    else {
                        httpdlog("INFO", "SSL ipv4 object created : " + std::to_string((unsigned long long int)ssl));
                    }
                    SSL_set_fd(ssl, sslclient);
                    httpdlog("INFO", "SSL ipv4 connection received");

                    int rc = -1;
                    int sslerror = 0;
                    bool isSslAccepted = false;
                    while (!(isSslAccepted)) {
                        rc = SSL_accept(ssl);
                        if (rc > 0) {
                            isSslAccepted = true;
                            httpdlog("INFO", "SSL ipv4 connection received ACCEPTED");
                            break;
                        }
                        else {
                            sslerror = SSL_get_error(ssl, rc);
                            if (rc == 0 && sslerror == SSL_ERROR_WANT_ACCEPT) {
                                //httpdlog("INFO", "SSL ipv4 connection received WANT ACCEPT");
                                sslerror = 0;
                                continue;
                            }
                            else if (rc < 0 && (sslerror == SSL_ERROR_WANT_WRITE || sslerror == SSL_ERROR_WANT_READ)) {
                                //httpdlog("INFO", "SSL connection received WANT WRITE OR READ");
                                sslerror = 0;
                                isSslAccepted = false;
                                continue;
                            }
                            else if (rc < 0) {
                                httpdlog("INFO", "SSL ipv4 connection  error");
                                isSslAccepted = false;
                                closesocket(sslclient);
                                SSL_shutdown(ssl);
                                SSL_free(ssl);
                                ssl = 0;
                                break;
                            }
                            else {
                                httpdlog("INFO", "SSL ipv4 connection error something else");
                                isSslAccepted = false;
                                closesocket(sslclient);
                                SSL_shutdown(ssl);
                                SSL_free(ssl);
                                ssl = 0;
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }

                    if (ssl && isSslAccepted) {
                        httpdlog("INFO", "Assigning task SSL ipv4 ");
                        //ThreadCommand *t = new ThreadCommand(WORK, sslclient, true, isSslAccepted, false, 0, "", ssl);
                        ThreadCommand* t = new ThreadCommand(WORK, sslclient, true, isSslAccepted, false, 0, "", ssl);
                        tp->assignTaskRr(t);
                    }
                }
            }

            if (pollfds[3].revents & POLLIN && transport & 0x02) {
                socklen_t addrlen = sizeof(sockaddr_in6);
                if ((sslclient6 = accept(sslsrv6, (sockaddr*)&sslclientAddr6, &addrlen))) {
                    SSL* ssl6 = SSL_new(cIp6);
                    if (!ssl6) {
                        httpdlog("INFO", "SSL ipv6 object creation failed");
                        continue;
                    }
                    else {
                        httpdlog("INFO", "SSL ipv6 object created : " + std::to_string((unsigned long long int)ssl6));
                    }
                    SSL_set_fd(ssl6, sslclient6);
                    httpdlog("INFO", "SSL ipv6 connection received");

                    int sslerror = 0;
                    //SSL *ssl = cmd->ssl;
                    bool isSslAccepted = false;
                    while (!(isSslAccepted)) {
                        rc = SSL_accept(ssl6);
                        if (rc > 0) {
                            isSslAccepted = true;
                            httpdlog("INFO", "SSL ipv6 connection received ACCEPTED");
                            break;
                        }
                        else {
                            sslerror = SSL_get_error(ssl6, rc);
                            if (rc == 0 && sslerror == SSL_ERROR_WANT_ACCEPT) {
                                //httpdlog("INFO", "SSL ipv6 connection received WANT ACCEPT");
                                sslerror = 0;
                                continue;
                            }
                            else if (rc < 0 && (sslerror == SSL_ERROR_WANT_WRITE || sslerror == SSL_ERROR_WANT_READ)) {
                                //httpdlog("INFO", "SSL connection received WANT WRITE OR READ");
                                sslerror = 0;
                                isSslAccepted = false;
                                continue;
                            }
                            else if (rc < 0) {
                                httpdlog("INFO", "SSL ipv6 connection error");
                                isSslAccepted = false;
                                closesocket(sslclient6);
                                SSL_shutdown(ssl6);
                                SSL_free(ssl6);
                                ssl6 = 0;
                                break;
                            }
                            else {
                                httpdlog("INFO", "SSL ipv6 connection error something else");
                                isSslAccepted = false;
                                closesocket(sslclient6);
                                SSL_shutdown(ssl6);
                                SSL_free(ssl6);
                                ssl6 = 0;
                                break;
                            }
                        }
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }


                    if (ssl6 && isSslAccepted) {
                        httpdlog("INFO", "Assigning task SSL ipv6 ");
                        ThreadCommand* t = new ThreadCommand(WORK, sslclient6, true, isSslAccepted, true, 0, "", ssl6);
                        tp->assignTaskRr(t);
                    }
                }
            }
            
            saveCount++;
            if (saveCount > 1000) {
                cookieManager.openFile();
                cookieManager.saveAll();
                cookieManager.closeFile();
                saveCount = 0;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        } else {
            httpdlog ( " ", "Looping in poll error" );
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    Py_END_ALLOW_THREADS
    Py_Finalize();

    return 0;
}
