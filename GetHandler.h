
#include "RequestHandler.h"
#ifndef CACHEPROXY_GETHANDLER_H
#define CACHEPROXY_GETHANDLER_H

class GetHandler {
public:
    void        handleGet(Rio& rio, char*, char*, char*, char*, Cache&, ClientInfo);
    CachePolicy getCacheControlTime(char*, int* );
    void        handleResponse(pair<CacheStatus, GetResponse>, Rio&, ClientInfo&, char* , char* , char*, char*, Cache&);
    CachePolicy handleResponseHeader(Rio&, Rio&, string&, char*, int*, int*, int* );
    void        handleResponseBody(Rio&, Rio&, char*, string&, int*, int);
    void        handleCache(ClientInfo , string , CachePolicy , int , Cache& , char* , char* );
    string      getETagIfAny(string);
    int         handleETag(char*, ClientInfo, char*, char*, GetResponse gr);
    void        sendCacheResponse(Rio&, GetResponse, ClientInfo);
};


#endif //CACHEPROXY_GETHANDLER_H
