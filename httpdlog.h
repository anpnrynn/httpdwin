//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#ifndef DEBUGLOG_H_INCLUDED
#define DEBUGLOG_H_INCLUDED

#include <iostream>
#include <string>
using namespace std;

void httpdlog(char *level, string info ){
    switch(level[0]){
    case 'D':
            cerr<<"DEBUG"<<" : "<<info<<endl;
        break;
    case 'I':
            cerr<<"INFO"<<"  : "<<info<<endl;
        break;
    case 'W':
            cerr<<"WARN"<<"  : "<<info<<endl;
        break;
    case 'E':
            cerr<<"ERROR"<<" : "<<info<<endl;
        break;
    case 'X':
            cerr<<"EXTRA"<<" : "<<info<<endl;
        break;
    case ' ':
            cerr<<"     "<<" : "<<info<<endl;
        break;
    default:
            cerr<<"     "<<" : "<<info<<endl;
        break;
    }
}

#endif // DEBUGLOG_H_INCLUDED
