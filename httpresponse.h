//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#ifndef HTTPRESPONSE_H_INCLUDED
#define HTTPRESPONSE_H_INCLUDED

#include <iostream>
#include <map>

using namespace std;
#include <httprequest.h>

class HttpResponse:public HttpRequest{
    public:
        string m_HttpStatusCode;
        string m_HttpStatusMessage;
};

#endif // HTTPRESPONSE_H_INCLUDED
