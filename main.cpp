//Copyright Anoop Kumar Narayanan - 2025 httpdwin
#include <iostream>
#include <atomic>
#include <exception>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <malloc.h>
#include <version.h>
#include <thread>
#include <chrono>
#include <debuglog.h>


using namespace std;

int main()
{
    debuglog(" ","Starting up...");
    SOCKET srv = 0, client = 0;
    SOCKET srv6 = 0, client6 = 0;
    SOCKET sslsrv =0, sslclient = 0;
    SOCKET sslsrv6 = 0, sslclient6 = 0;

    short int serverPort  = 0;
    short int serverPort6 = 0;
    short int sslserverPort = 0;
    short int sslserverPort6 = 0;

    WSADATA wsaData;  // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
            printf("WSAStartup failed\n"); return 1;
    }


    sockaddr_in  srvAddr;
    sockaddr_in  clientAddr;

    sockaddr_in6 srvAddr6;
    sockaddr_in6 clientAddr6;

    sockaddr_in  sslsrvAddr;
    sockaddr_in  sslclientAddr;

    sockaddr_in6 sslsrvAddr6;
    sockaddr_in6 sslclientAddr6;

    //debuglog (  "INFO"," Creating socket(s) \n" );
    srv = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
    srvAddr.sin_family  = AF_INET;
    srvAddr.sin_addr.s_addr     = 0x00000000;
    srvAddr.sin_port   = htons ( serverPort );
    //debuglog (  "INFO"," IPv4 Socket created successfully : Port = %d \n", SRVPORT );

    srv6 = socket(PF_INET6,  SOCK_STREAM, IPPROTO_TCP);
    memset(&srvAddr6, 0, sizeof(struct sockaddr_in6));
    srvAddr6.sin6_family = AF_INET6;

    srvAddr6.sin6_addr = in6addr_loopback;
    srvAddr6.sin6_port = htons ( serverPort6 );
    //debuglog (  "INFO: IPv6 Socket created successfully : Port = %d \n", SRVPORT6 );

    //debuglog (  "INFO: Creating ssl socket(s) \n" );
    sslsrv = socket(PF_INET,  SOCK_STREAM, IPPROTO_TCP);
    sslsrvAddr.sin_family  = AF_INET;
    sslsrvAddr.sin_addr.s_addr     = 0x00000000;
    sslsrvAddr.sin_port   = htons ( sslserverPort );
    //debuglog (  "INFO: SSL IPv4 Socket created successfully : Port = %d \n", SSLSRVPORT );

    sslsrv6 = socket(PF_INET6,  SOCK_STREAM, IPPROTO_TCP);
    memset(&sslsrvAddr6, 0, sizeof(struct sockaddr_in6));
    sslsrvAddr6.sin6_family = AF_INET6;

    sslserverPort6 = sslserverPort;
    sslsrvAddr6.sin6_port = htons ( sslserverPort6 );
    //debuglog (  "INFO: SSL IPv6 Socket created successfully : Port = %d \n", SSLSRVPORT6 );


    int sockflag = 1;
    int sockret = setsockopt ( srv, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        //debuglog (  "ERRR: Unable to setsockopt - IPv4 \n" );
        return 1;
    }

    sockflag = 1;
    sockret = setsockopt ( srv6, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        //debuglog (  "ERRR: Unable to setsockopt - IPv6 \n" );
        return 1;
    }

#ifdef USE_SSL
    sockflag = 1;
    sockret = setsockopt ( sslsrv, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        debuglog (  "ERRR: Unable to setsockopt - SSLIPv4 \n" );
        return 1;
    }

    sockflag = 1;
    sockret = setsockopt ( sslsrv6, SOL_SOCKET, SO_REUSEADDR , (char *)&sockflag, sizeof ( sockflag ) );

    if ( sockret == -1 ) {
        debuglog (  "ERRR: Unable to setsockopt - SSLIPv6 \n" );
        return 1;
    }
#endif


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

    //debuglog (  "INFO: Bound successfully both IPv4 and IPv6 \n" );

    if ( listen ( srv, 20 ) != 0 ) {
        //debuglog (  "ERRR: Unable to setup backlog - IPV4\n" );
    }

    if ( listen ( srv6, 20 ) != 0 ) {
        //debuglog (  "ERRR: Unable to setup backlog - IPv6\n" );
    }

    if ( listen ( sslsrv, 20 ) != 0 ) {
        //debuglog (  "ERRR: Unable to setup backlog - SSL IPV4\n" );
    }

    if ( listen ( sslsrv6, 20 ) != 0 ) {
        //debuglog (  "ERRR: Unable to setup backlog - SSL IPv6\n" );
    }

    unsigned long mode = 1;
    ioctlsocket(srv, FIONBIO, &mode);
    ioctlsocket(srv6, FIONBIO, &mode);
    ioctlsocket(sslsrv, FIONBIO, &mode);
    ioctlsocket(sslsrv6, FIONBIO, &mode);

    return 0;
}
