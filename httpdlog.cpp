//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#define _CRT_SECURE_NO_WARNINGS
#include <httpdlog.h>
#include <mutex>
using namespace std;

#define _CRT_SECURE_NO_WARNINGS

int httpdloglevel = 4;

mutex logmutex;

void httpdlog(const char *level, string info ){
    logmutex.lock();
    switch(level[0]){
    case 'D':
        if( httpdloglevel >= 3)
            cerr<<"DEBUG"<<" : "<<info<<endl;
        break;
    case 'I':
        if( httpdloglevel >= 2)
            cerr<<"INFO"<<"  : "<<info<<endl;
        break;
    case 'W':
        if( httpdloglevel >= 1)
            cerr<<"WARN"<<"  : "<<info<<endl;
        break;
    case 'E':
        if( httpdloglevel >= 0)
            cerr<<"ERROR"<<" : "<<info<<endl;
        break;
    case 'X':
        if( httpdloglevel >= 4)
            cerr<<"XTRA "<<" : "<<info<<endl;
        break;
    case ' ':
        if( httpdloglevel >= 0)
            cerr<<"     "<<" : "<<info<<endl;
        break;
    default:
        if( httpdloglevel >= 4)
            cerr<<"     "<<" : "<<info<<endl;
        break;
    }
    logmutex.unlock();
}
