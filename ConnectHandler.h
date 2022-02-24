
#ifndef CACHEPROXY_CONNECTHANDLER_H
#define CACHEPROXY_CONNECTHANDLER_H
#include "RequestHandler.h"

/**
 * Handle CONNECT request
 */
class ConnHandler{
public:
    void handleConn(Rio& rio, char*, char*, char*, char*, ClientInfo);
    void monitorSocketSet(Rio&, Rio&, ClientInfo);
    int transferData(int sender, int receiver);
};

#endif //CACHEPROXY_CONNECTHANDLER_H
