//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#define _CRT_SECURE_NO_WARNINGS
#include <httprequest.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;


#include <httpdlog.h>

#define _CRT_SECURE_NO_WARNINGS

typedef vector<string> VectorStr;
extern std::map<string,string> httpHeaders;


NameMimeValues & NameMimeValues::operator =( const NameMimeValues& cp){
    m_Name  = cp.m_Name;
    m_Mime  = cp.m_Mime;
    m_Value = cp.m_Value;
    return *this;
}

HttpRequest::HttpRequest(){
    m_Len = 0;
    m_cLen = 0;
    m_FieldCount = 0;
    m_HttpHeaderComplete = false;
    m_CookieList = 0;
}

HttpRequest::~HttpRequest(){
    httpdlog("DEBUG", "Deleting request object: " + to_string((unsigned long long int) this));
    m_Len = 0;
    m_cLen = 0;
}


void HttpRequest::decodeUrl(){
    std::ostringstream decoded;
    for (size_t i = 0; i < m_EncodedUrl.length(); ++i) {
        if (m_EncodedUrl[i] == '%' && i + 2 < m_EncodedUrl.length()) {
            std::istringstream hexStream(m_EncodedUrl.substr(i + 1, 2));
            int hexValue;
            hexStream >> std::hex >> hexValue;
            decoded << static_cast<char>(hexValue);
            i+=2;
        } else if (m_EncodedUrl[i] == '+') {
            decoded << ' ';
        } else {
            decoded << m_EncodedUrl[i];
        }
    }
    m_DecodedUrl = decoded.str();
}

void HttpRequest::parseQuerystring(){
    size_t i = 0;
    while ( m_DecodedUrl[i] != '?' && i < m_DecodedUrl.length() ){
        m_RequestFile += m_DecodedUrl[i++];
    }
    i++;
    httpdlog("WARN", "Request File = " + m_RequestFile );
    NameMimeValues *nmv = new NameMimeValues;
    nmv->m_Name = "";
    nmv->m_Mime = "";
    nmv->m_Value = "";
    nmv->m_TempFileName = "";
    string *data = &(nmv->m_Name);
    while ( i < m_DecodedUrl.length() ){
        if( m_DecodedUrl[i] == '=' ){
            data = &(nmv->m_Value);
            i++;
        } else if( m_DecodedUrl[i] == '&') {
            //query.insert(new std::pair<string,NameMimeValues>(nmv.m_Name, nmv) );
            query[nmv->m_Name] = nmv;
            //httpdlog("INFO ", nmv->m_Name + " = " +nmv->m_Value );
            nmv= new NameMimeValues;
            nmv->m_Name = "";
            nmv->m_Mime = "";
            nmv->m_Value = "";
            nmv->m_TempFileName = "";
            data = &(nmv->m_Name);
            i++;
        } else {
            *data += m_DecodedUrl[i];
            i++;
        }
    }

    if(data && data->length() > 0){
        query[nmv->m_Name] = nmv;
    }

    Query::iterator qi = query.begin();
    httpdlog("DEBUG", "Query String = ");
    while( qi != query.end() ){
        httpdlog("DEBUG", qi->first + "  : " +(*(qi->second)).m_Value );
        qi++;
    }
}

void HttpRequest::parsePostDataQueryString(){
}

void HttpRequest::readHttpReqLine( HttpRequest *req, string &line ){
    size_t m    = line.find(' ', 0 );
    if (m == std::string::npos){
        httpdlog("WARN","HTTP header request line malformed (Method part)");
        return;
    }
    req->m_Method  = line.substr(0,m );
    size_t n       = line.find(' ', m+1);
    if (n == std::string::npos){
        httpdlog("WARN","HTTP header request line malformed (Url part)");
        return;
    }
    //cerr<<m<<", "<<n<<endl;
    req->m_EncodedUrl = line.substr( m+1, n-m-1);
    req->m_Version = line.substr( n+1, line.length()-n-1);

    req->decodeUrl();
    req->parseQuerystring();
    httpdlog(" ", req->m_Method +"\n        "+req->m_Version +"\n        "+ req->m_EncodedUrl+"\n        "+req->m_DecodedUrl);
}

CookieList* HttpRequest::readCookies ( HttpRequest *req, string &line ){
    size_t i = 0;
	CookieList* cookieList = new CookieList;
    Cookie c;

    string cookieName = "";
    string cookieValue = "";
    bool readingName = true;
    while( i < line.length() ){
        if( line[i] == '=' && readingName ){
            readingName = false;
            i++;
        } else if( line[i] == ';' ){
            
            c.m_name = cookieName;
            c.m_value = cookieValue;
			cookieList->push_back(c);
            httpdlog("DEBUG", "Cookie: " + cookieName + " -> " + cookieValue);
            cookieName = "";
            cookieValue = "";
            readingName = true;
            i++;
            if( line[i] == ' ' ) i++;
        } else {
            if( readingName )
                cookieName += line[i];
            else
                cookieValue += line[i];
            i++;
        }
    }
    if( cookieName.length() > 0 ){
        c.m_name = cookieName;
        c.m_value = cookieValue;
        cookieList->push_back(c);
        httpdlog("DEBUG", "Cookie: " + cookieName + " -> " + cookieValue);
    }

	CookieList::iterator it = cookieList->begin();
    string gname = "";
    while (it != cookieList->end()) {
        if (it->m_name == "SessionID") {
            gname = it->m_value;
            break;
        }
        it++;
    }

    it = cookieList->begin();
    while (it != cookieList->end()) {
        it->m_gname = gname;
        it++;
    }
	return cookieList;
}


void HttpRequest::readHeaderLine ( HttpRequest *req, string &line ){
    //httpdlog("WARN","HTTP header field line processing");
    size_t m    = line.find(':', 0 );
    if (m == std::string::npos){
        httpdlog("WARN","HTTP header field line empty or malformed : "+line);
        return;
    }
    string field     = line.substr(0, m );
    string fieldName = "";


    std::map<string,string>::iterator it = httpHeaders.find( field );
    if( it == httpHeaders.end() )
        fieldName = "";
    else
        fieldName = it->second;
    /*
    try{
        fieldName = httpHeaders[ field ];
    } catch ( exception e ){
        fieldName = "";
    }
    */

    if (field == "Content-Length") {
		
        req->m_cLen = atoi(line.substr(m + 2, line.length() - 1).c_str());
        httpdlog("DEBUG", "Post or Put data present , Content-Length = " + to_string(req->m_cLen)) ;
    }

    if( fieldName != "" ){
		string fieldValue = line.substr(m + 2, line.length() - 1);
        req->m_HeaderNames[req->m_FieldCount] = field;
        req->m_Headers[req->m_FieldCount++] = fieldValue;

        if (field == "Cookie") {
            req->m_CookieList = readCookies(req, fieldValue);
        }

        httpdlog("DEBUG", req->m_HeaderNames[req->m_FieldCount-1] + " -> "+ req->m_Headers[req->m_FieldCount-1]);
    } else {
        httpdlog("XTRA","HTTP header field unknown : " + field );
    }
}

void HttpRequest::readHttpHeader ( HttpRequest *req, char *buffer, int *len, int totalLen){
    VectorStr vs;
    char *line = buffer;
    size_t i = 0;
    *len = 0;
    while( i < totalLen ){
        if( buffer[i] == '\r'){
            buffer[i] = 0;
            string s = line;
            //cerr<<"---->"<<line<<"  <>  "<<s<<endl;
            vs.push_back(s);
            line = &buffer[i+2];
            if( buffer[i+1] =='\n' && buffer[i+2] =='\r' && buffer[i+3] == '\n' ){
                *len = i+4;
                req->m_HttpHeaderComplete = true;
                break;
            }
            i++;
        }
        i++;
    }

    HttpRequest::readHttpReqLine(req, vs[0]);
    i = 0;
    while( i < vs.size()-1 ){
        i++;
        HttpRequest::readHeaderLine(req, vs[i]);
    }
}

void HttpRequest::readHttpData   ( char *buffer, int *len, int totalLen, string *filename ){
}

void HttpRequest::processPostData( string *filename ){
}

void HttpRequest::processMultipartData ( string *filename ){
}

