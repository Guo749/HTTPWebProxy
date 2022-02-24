#ifndef CACHEPROXY_REQUESTHANDLER_H
#define CACHEPROXY_REQUESTHANDLER_H

#include "utilities.h"
#include "Rio.h"
#include "Cache.h"
#include "WorkBuffer.h"


/* command method used by different request processing header file */
int processHeader(Rio& rio, char* clientRequest, char* method, char* uri, char* version, char*, char*);

void setupHostAndPort(char* clientRequest, char* hostName, char* portNum, char* uri) ;

void sendErrorHTML(Rio& rio, string html, ClientInfo);

int getContentLength(char* clientRequest);

string readAndWriteChunkedData(Rio&, Rio&);

void readAndWriteContentLengthData(Rio&, Rio&, int, char*);

void printRequest(char*, int);

void sendBadRequest(Rio&, ClientInfo);

void sendGatewayTimeout(Rio& rio, ClientInfo info, char*, char*);

#endif //CACHEPROXY_REQUESTHANDLER_H
