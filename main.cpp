//Copyright Anoop Kumar Narayanan - 2025 httpdwin
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
            int m = line.find('=', 1 );
            int n = line.find('\n', 1);
            string name  = line.substr(0,m );
            string value = line.substr(m+1, n );
            if( value[value.length()-1] == '\r' )
                value[value.length()-1] = 0;
            if( name != "" && value != "" )
                httpdwinConfig[name ]= value;
            httpdlog("DEBUG", name +" = "+ value + " value read." );
        }
    } else {
        return 1;
    }
    f.close();
    return 0;
}


int main()
{
    httpdlog(" ","Starting up...");

    parseConfig();

    SOCKET srv = 0, client = 0;
    SOCKET srv6 = 0, client6 = 0;
    SOCKET sslsrv =0, sslclient = 0;
    SOCKET sslsrv6 = 0, sslclient6 = 0;


    short int serverPort  = atoi(httpdwinConfig["httpport"].c_str());
    short int serverPort6 = serverPort;
    short int sslserverPort = atoi(httpdwinConfig["httpsport"].c_str());
    short int sslserverPort6 = sslserverPort6;

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

    httpdlog (  "INFO","Creating socket(s) \n" );
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

    httpdlog (  "INFO","Creating ssl socket(s) \n" );
    sslsrv = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
    sslsrvAddr.sin_family  = AF_INET;
    sslsrvAddr.sin_addr.s_addr     = 0x00000000;
    sslsrvAddr.sin_port   = htons ( sslserverPort );
    httpdlog (  "INFO","SSL IPv4 Socket created successfully" );

    sslsrv6 = socket(PF_INET6,  SOCK_STREAM, IPPROTO_TCP);
    memset(&sslsrvAddr6, 0, sizeof(struct sockaddr_in6));
    sslsrvAddr6.sin6_family = AF_INET6;

    sslserverPort6 = sslserverPort;
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
            break;
        } else {
            count++;
        }
    }
    count = 0;

    while ( bind ( srv6, ( const sockaddr * ) &srvAddr6, sizeof ( sockaddr_in6 ) ) == SOCKET_ERROR ){
        int errorno = errno;
        char address[64];
        inet_ntop( AF_INET6, &(srvAddr6.sin6_addr), address , 64 );
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            break;
        } else {
            count++;
        }
    }

    count = 0;
    while ( bind ( sslsrv, ( const sockaddr * ) &sslsrvAddr, sizeof ( sockaddr_in ) ) == SOCKET_ERROR ) {
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            break;
        } else {
            count++;
        }
    }

    count = 0;
    while ( bind ( sslsrv6, ( const sockaddr * ) &sslsrvAddr6, sizeof ( sockaddr_in6 ) ) == SOCKET_ERROR ) {
        std::this_thread::sleep_for ( std::chrono::microseconds ( 100000 ) );
        if ( count > 10 ) {
            break;
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

    if ( SSL_CTX_use_certificate_file ( cIp6, "C:\\Httpdwin\\Certs\\httpdwincert.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Certificate file issue IPv6 C:\\Httpdwin\\Certs\\httpdwincert.pem" );
        return ( 1001 );
    } else {
        httpdlog (  "INFO", "Certificate file loaded C:\\Httpdwin\\Certs\\httpdwincert.pem" );
    }

    if ( SSL_CTX_use_PrivateKey_file ( cIp6,  "C:\\Httpdwin\\Certs\\httpdwinkey.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Private key file issue IPv6 C:\\Httpdwin\\Certs\\httpdwinkey.pem" );
        return ( 1002 );
    } else {
        httpdlog (  "INFO", "Private key file loaded C:\\Httpdwin\\Certs\\httpdwinkey.pem" );
    }

    WSAPOLLFD    *pollfds = new WSAPOLLFD[5];
    pollfds[0].fd = srv;
    pollfds[0].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    pollfds[0].revents = 0;

    pollfds[1].fd = srv6;
    pollfds[1].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    pollfds[1].revents = 0;

    pollfds[2].fd = sslsrv;
    pollfds[2].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    pollfds[2].revents = 0;

    pollfds[3].fd = sslsrv6;
    pollfds[3].events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    pollfds[3].revents = 0;

    int nPorts   = 4;
    int nThreads = atoi( httpdwinConfig["threads"].c_str() );

    snprintf(logMsg,254, "Starting %d Threads ", nThreads);
    httpdlog("INFO", logMsg);

    ThreadPool *tp = new ThreadPool(nThreads);

    while ( true ) {
        fflush ( stderr );

        int i = 0;
        while( i < nPorts ){
            pollfds[i].events = POLLIN;
            i++;
        }

        int rc = 0;

        if ( ( rc = WSAPoll ( pollfds, nPorts, 1 ) ) != SOCKET_ERROR ) {
            httpdlog ( "Debug", "Looping in poll" );
            if ( pollfds[0].revents & POLLIN ) {
                socklen_t addrlen = sizeof ( sockaddr_in );
                if ( ( client = accept ( srv, ( sockaddr * ) &clientAddr, &addrlen ) ) ) {
                        tp->assignTask(new ThreadCommand(WORK, client) );
                }
            }

            if ( pollfds[1].revents & POLLIN ) {
                socklen_t addrlen = sizeof ( sockaddr_in );
                if ( ( client6 = accept ( srv, ( sockaddr * ) &clientAddr, &addrlen ) ) ) {
                        tp->assignTask(new ThreadCommand(WORK, client6) );
                }
            }

            if ( pollfds[2].revents & POLLIN ) {
                socklen_t addrlen = sizeof ( sockaddr_in );
                if ( ( sslclient = accept ( srv, ( sockaddr * ) &clientAddr, &addrlen ) ) ) {
                        tp->assignTask(new ThreadCommand(WORK, sslclient) );
                }
            }

            if ( pollfds[3].revents & POLLIN ) {
                socklen_t addrlen = sizeof ( sockaddr_in );
                if ( ( sslclient6 = accept ( srv, ( sockaddr * ) &clientAddr, &addrlen ) ) ) {
                        tp->assignTask(new ThreadCommand(WORK, sslclient6) );
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        } else {
            httpdlog ( " ", "Looping in poll error" );
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return 0;
}
