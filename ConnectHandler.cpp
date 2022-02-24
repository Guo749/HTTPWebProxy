
#include "ConnectHandler.h"


/**
 * Conn Handler
 *
 */
void ConnHandler::handleConn(Rio &rio, char *clientRequest, char *method, char *uri, char *version, ClientInfo info) {
    /* step1: process connect request */
    Log myLog;

    /* step2: connect with the backend server */
    char hostName[MAXLINE];
    char portNum[MAXLINE];

    int rv = processHeader(rio, clientRequest, method, uri, version, hostName, portNum);
    if(rv == 0){
        sendBadRequest(rio, info);
        return;
    }

    int clientfd = createClientFd(hostName, portNum);
    Rio rioServer(clientfd);

    if (clientfd == -1) {
        sendGatewayTimeout(rio, info, hostName, portNum);
        return;
    }

    myLog.write(info.UUID + ": " + "connection with remote server has established");


    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.1 200 OK\r\n\r\n");
    rio.Rio_writen(buf, strlen(buf));
    myLog.write(info.UUID + ": " + "writing 200 OK to client");

    monitorSocketSet(rio, rioServer, info);
    myLog.write(info.UUID + ": " + "Tunnel closed");
    myLog.write(info.UUID + ": " + "finished, return");
}

/**
 * IO Multiplexing for rio & rioServer
 *
 * @param rio  client we connect
 * @param rioServer remote server we connect
 * @param info client info section
 */
void ConnHandler::monitorSocketSet(Rio &rio, Rio &rioServer, ClientInfo info) {
    Log myLog;

    while (1) {
        fd_set socketSet;
        FD_ZERO(&socketSet);
        int maxfd = rio.rio_fd > rioServer.rio_fd ? rio.rio_fd : rioServer.rio_fd;

        /* add to socket set */
        FD_SET(rio.rio_fd, &socketSet);
        FD_SET(rioServer.rio_fd, &socketSet);

        struct timeval time;
        time.tv_sec = 0;
        time.tv_usec = 1000000000;

        int ret = select(maxfd + 1, &socketSet, NULL, NULL, &time);

        if (ret <= 0)
            break;

        /* identify event */
        if (FD_ISSET(rio.rio_fd, &socketSet)) {
            int res = transferData(rio.rio_fd, rioServer.rio_fd);
            myLog.write(info.UUID + ": " + "connection ends from client side");

            if (res == -1) {
                break;
            }
        } else {
            int res = transferData(rioServer.rio_fd, rio.rio_fd);
            myLog.write(info.UUID + ": " + "connection ends from server side");

            if (res == -1) {
                break;
            }
        }
    }
}

/**
 * Transfer Data between sender  and receiver
 * first we get data from sender, then we send it to receiver without touching it
 *
 * @param sender who sends the data to proxy
 * @param receiver proxy sends data to who
 * @return 0 means connection continues, -1 means broken pipe, connection ends
 */
int ConnHandler::transferData(int sender, int receiver) {
    vector<char> buf;
    buf.resize(MAXLINE);
    ssize_t total = 0;
    ssize_t n = 0;

    total = n;
    while ((n = read(sender, &buf[total], MAXLINE)) == MAXLINE) {
        buf.resize(total + 2 * MAXLINE);
        total += n;
    }

    total += n;
    buf.resize(total);

    if(buf.empty()){
        return -1;
    }

    int status = write(receiver, &buf.data()[0], buf.size());
    if (status == -1) {
        return -1;
    }

    return 0;
}