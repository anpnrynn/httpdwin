//Copyright Anoop Kumar Narayanan - 2025 //httpdwin
#ifndef HTTPDLOG_H_INCLUDED
#define HTTPDLOG_H_INCLUDED
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
using namespace std;



#include <largefile.h>



extern int httpdloglevel;

void httpdlog(const char *level, string info );

#endif // DEBUGLOG_H_INCLUDED
