//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#ifndef HTTPRESPONSE_H_INCLUDED
#define HTTPRESPONSE_H_INCLUDED

#include <iostream>
#include <map>

using namespace std;
#include <httprequest.h>

class HttpResponse:public HttpRequest{
    public:
        string m_ResponseHeader;

        string m_StatusCode;
        string m_StatusMessage;
        string m_ContentLength;
        string m_ContentType;
        string m_HttpData;

        int    m_ResponseHeaderLen;

        HttpResponse(string statusCode, string statusMessage, string contentLength, string contentType, string filename = "" );
        void BuildResponseHeader(  std::map<string,string> *headers  );
        void addResponseData( string & data );

        static HttpResponse *CreateSimpleResponse( string filename );
};

#endif // HTTPRESPONSE_H_INCLUDED
