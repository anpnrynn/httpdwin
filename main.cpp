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
#include <httpdlog.h>

#include <openssl/ssl.h>
#include <openssl/err.h>


using namespace std;



int main()
{
    httpdlog(" ","Starting up...");
    SOCKET srv = 0, client = 0;
    SOCKET srv6 = 0, client6 = 0;
    SOCKET sslsrv =0, sslclient = 0;
    SOCKET sslsrv6 = 0, sslclient6 = 0;

    short int serverPort  = 8080;
    short int serverPort6 = 8080;
    short int sslserverPort = 8081;
    short int sslserverPort6 = 8081;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
            httpdlog("ERROR", " WSAStartup failed\n"); return 1;
    }


    sockaddr_in  srvAddr;
    sockaddr_in  clientAddr;

    sockaddr_in6 srvAddr6;
    sockaddr_in6 clientAddr6;

    sockaddr_in  sslsrvAddr;
    sockaddr_in  sslclientAddr;

    sockaddr_in6 sslsrvAddr6;
    sockaddr_in6 sslclientAddr6;

    //httpdlog (  "INFO"," Creating socket(s) \n" );
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

    if ( SSL_CTX_use_certificate_file ( cIp4, "C:\\Certs\\httpdwincert.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Certificate file issue C:\\Certs\\httpdwincert.pem" );
        return ( 1001 );
    } else {
        httpdlog (  "INFO", "Certiticate file loaded C:\\Certs\\httpdwincert.pem" );
    }

    if ( SSL_CTX_use_PrivateKey_file ( cIp4,"C:\\Certs\\httpdwinkey.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Private key file issue C:\\Certs\\httpdwinkey.pem" );
        return ( 1002 );
    } else {
        httpdlog (  "INFO", "Private key file loaded C:\\Certs\\httpdwinkey.pem" );
    }

    const SSL_METHOD *mIp6;
    SSL_CTX *cIp6;
    mIp6 = TLS_server_method();
    cIp6 = SSL_CTX_new ( mIp6 );

    if ( !cIp6 ) {
        httpdlog (  "ERROR", "Unable to create SSL context" );
        return ( 1000 );
    }

    if ( SSL_CTX_use_certificate_file ( cIp6, "C:\\Certs\\httpdwincert.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Certificate file issue  C:\\Certs\\" );
        return ( 1001 );
    } else {
        httpdlog (  "INFO", "Certificate file loaded C:\\Certs\\httpdwincert.pem" );
    }

    if ( SSL_CTX_use_PrivateKey_file ( cIp6,  "C:\\Certs\\httpdwinkey.pem", SSL_FILETYPE_PEM ) <= 0 ) {
        httpdlog (  "ERROR", "Private key file issue C:\\Certs\\" );
        return ( 1002 );
    } else {
        httpdlog (  "INFO", "Private key file loaded C:\\Certs\\httpdwinkey.pem" );
    }

    return 0;
}
