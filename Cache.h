#ifndef CACHEPROXY_CACHE_H
#define CACHEPROXY_CACHE_H

#include "utilities.h"
#include "HttpResponse.h"
#include "WorkBuffer.h"

/**
 * Basic structure for Doubly Linked List
 */
class Node{
public:

    /* host name */
    string host;

    /* unique identifier */
    string uri;

    string eTag;

    /* Response for this request */
    GetResponse gr;
    Node* next;
    Node* prev;


    Node(){}
    Node(string hhh, string uuu, GetResponse ggg):
        host(hhh), uri(uuu), gr(ggg), next(nullptr), prev(nullptr){}

};

/**
 * Structure is like doubly linked list
 * maintained by head & tail
 * todo: lru + map
 */
class Cache {
public:
    Cache();
    pair<CacheStatus, GetResponse> ReadFromCache(string, string, ClientInfo);
    int writeToCache(string, string, GetResponse);
    void printCache();
    void clearAllInvalidCache(string, string);
    void deleteNode(Node* );
    void addNode(Node* );

private:
    Node* head;
    Node* tail;

    /****** concurrency control
        this version favors reader over writer ********/

    /* number of reader */
    int numReader;

    /* cache size cannot be larger than MAX_CACHE_SIZE */
    int totalSize;

    /* mutex guarded the entry of cache */
    sem_t mutex;

    /* mutex guarded the right to write */
    sem_t W;
};


#endif //CACHEPROXY_CACHE_H
