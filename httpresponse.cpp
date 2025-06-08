//Copyright Anoop Kumar Narayanan - 2025 //httpdwin

#include <iostream>


#include <string.h>


#include <httpresponse.h>
#include <version.h>

HttpResponse::HttpResponse(string statusCode, string statusMessage, string contentLength, string contentType, string filename ){
    m_Version = "HTTP/1.1";
    m_StatusCode = statusCode;
    m_StatusMessage = statusMessage;
    m_ContentType = contentType;
    m_ContentLength = contentLength;
    m_Filename = filename;
    m_HttpData = m_Version + " " + statusCode + " " + statusMessage + " " + filename;
}

HttpResponse *HttpResponse::CreateSimpleResponse( string filename ){
    HttpResponse *resp = new HttpResponse("200", "OK", "", "text/plain", filename );
    resp->m_ContentLength = std::to_string( (resp->m_HttpData).length() );
    return resp;
}

void HttpResponse::BuildResponseHeader( std::map<string,string> *headers ){
    int n = 0;

    strcpy((char*)m_Buffer, m_Version.c_str());
    n = m_Version.length();

    strcat((char*)&m_Buffer[n], " ");
    n++;
    strcpy((char*)&m_Buffer[n], m_StatusCode.c_str());
    n+= m_StatusCode.length();

    strcat((char*)&m_Buffer[n], " ");
    n++;
    strcpy((char*)&m_Buffer[n], m_StatusMessage.c_str());
    n+= m_StatusMessage.length();

    strcat((char*)&m_Buffer[n], "\r\n");
    n += 2;

    char *server = "Server: " HTTPDWIN_NAME "-" HTTPDWIN_VERSION;

    strcpy((char*)&m_Buffer[n], server);
    n += strlen(server);
    strcat((char*)&m_Buffer[n], "\r\n");
    n += 2;

    strcpy((char*)&m_Buffer[n], "Content-Length:");
    n += strlen( "Content-Length:" );

    strcpy((char*)&m_Buffer[n], m_ContentLength.c_str());
    n += m_ContentLength.length();
    strcat((char*)&m_Buffer[n], "\r\n");
    n += 2;

    strcpy((char*)&m_Buffer[n], "Content-Type:");
    n += strlen( "Content-Type:" );

    strcpy((char*)&m_Buffer[n], m_ContentType.c_str());
    n += m_ContentType.length();
    strcat((char*)&m_Buffer[n], "\r\n");
    n += 2;

    if( headers ){
        int j = 1;
        map<string,string>::iterator i = headers->begin();
        while( i != headers->end() ){
            strcpy((char*)&m_Buffer[n], i->first.c_str() );
            n += i->first.length();
            strcpy((char*)&m_Buffer[n], i->second.c_str());
            n += i->second.length();
            strcat((char*)&m_Buffer[n], "\r\n");
            n += 2;
            i++;
        }
    }

    strcat((char*)&m_Buffer[n], "\r\n");
    n += 2;

    m_ResponseHeaderLen = n;
}

void HttpResponse::addResponseData( string & data ){
    strcpy( (char*)(&m_Buffer[m_ResponseHeaderLen]), data.c_str() );
}
