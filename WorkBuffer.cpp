#include "WorkBuffer.h"

SodaMachine::SodaMachine(int cap){
    this->capacity = cap;
    this->buf      = vector<ClientInfo>(cap);
    this->front    = 0;
    this->rear     = 0;
    sem_init(&this->mutex, 0, 1);
    sem_init(&this->slots, 0, cap); /* how many items we want in our buffer */
    sem_init(&this->items, 0, 0);
}

/**
 * @brief Master thread is inserting one client request 
 *        waiting for the worker to grasp one of them
 *        if there is no available slots, master will be blocked
 * 
 * @param item 
 */
void SodaMachine::insertFd(ClientInfo info){
    P(&this->slots);                          
    P(&this->mutex);                          
    
    this->buf[(++this->rear)%(this->capacity)] = info;

    V(&this->mutex);                          
    V(&this->items);                          
}

/**
 * @brief One free worker will get one item from the sodamachine to work
 *        if there is no avaible items, workers will be blocked
 * 
 * @return int the client file descriptor for this worker
 */
ClientInfo SodaMachine::removeFd(){
    ClientInfo info;
    P(&this->items);                          
    P(&this->mutex);

    info = this->buf[(++this->front)%(this->capacity)];

    V(&this->mutex);                          
    V(&this->slots);                         
    return info;
}

ClientInfo::ClientInfo(string u, string I, int f, string arr) {
    this->UUID        = u;
    this->IPAddr      = I;
    this->fd          = f;
    this->arrivalTime = arr;
}
