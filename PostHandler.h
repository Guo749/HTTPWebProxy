#ifndef CACHEPROXY_POSTHANDLER_H
#define CACHEPROXY_POSTHANDLER_H
#include "RequestHandler.h"

/**
 * Handle Post Request
 */
class PostHandler{
public:
    void handlePost(Rio& rio, char*, char*, char*, char*, ClientInfo info);
    void handleResponse(Rio& rio, int , char* , char* , ClientInfo );
    int checkChunked(char* );
};



#endif //CACHEPROXY_POSTHANDLER_H
