#ifndef PROXY_WORKERBUFFER_H
#define PROXY_WORKERBUFFER_H
#include "utilities.h"

struct ClientInfo{

    /* the unique identifier for the client request */
    string UUID;

    /* the IP address that client made request from */
    string IPAddr;

    /* file descriptor for this client */
    int fd;

    /* the time when the request arrived */
    string arrivalTime;

    /* used for printing the log, assign later */
    string request;

    ClientInfo(){}
    ClientInfo(string u, string I, int f, string arr);
};

/**
 * @brief Consider each file descriptor as a soda in a soda machine
 *          every client will wait for one worker to serve him/her
 */
class SodaMachine{
public:
    vector<ClientInfo> buf;
    int capacity;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;

    SodaMachine(int);
    void        insertFd(ClientInfo info);
    ClientInfo  removeFd();


};

#endif