#ifndef CACHEPROXY_HTTPRESPONSE_H
#define CACHEPROXY_HTTPRESPONSE_H

#include "utilities.h"

/**
 * Response class, can be extened by GET, POST,CONNECT
 */
class Response{
public:

    /* response header + body */
    string response;
    Response() : response("") {}
    explicit Response(string req) :response(req) {}
};

class GetResponse : public Response{
public:

    /* how long the cache can be valid ? */
    int validTime;

    /* since when it was writen into, format in second*/
    int writeTime;

    string eTag;

    GetResponse() : Response(""), validTime(0), writeTime(0){}
    GetResponse(int t, int w, string r) : Response(r), validTime(t), writeTime(w){}
};


#endif //CACHEPROXY_HTTPRESPONSE_H
