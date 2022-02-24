#include "Cache.h"

/**
 * Constructor for cache
 *
 * @param mutex guard the this->numReader
 * @param W guard the right to write
 * @param numReader how many readers we have
 */
Cache::Cache() :head(new Node()), tail(new Node()), numReader(0), totalSize(0) {
    this->head->next = this->tail;
    this->tail->prev = this->head;

    sem_init(&this->mutex, 0, 1);
    sem_init(&this->W    , 0, 1);
}

/**
 * Searching corresponding content from the cache
 * according to the uri & host
 *
 * @param host the host we are looking for, like www.cs.duke.edu
 * @param uri the resource locator, like /index.html
 * @param flag
 *          1 -> valid
 *          2 -> not cached
 *          3 -> cached, but expires
 *
 * @return the content we get, "" if we get nothing, the full html file if we have one
 */
pair<CacheStatus, GetResponse> Cache::ReadFromCache(string host, string uri, ClientInfo clientInfo){
    GetResponse gr;
    CacheStatus cs = CS_CACHE_NOT_FOUND;
    Log myLog;

    P(&this->mutex);

    this->numReader++;
    /** first reader, waiting to prevent any writer */
    if(this->numReader == 1){
        P(&W);
    }

    V(&this->mutex);

    /********************************** critical section begins *******************************************************/
    TimeTools tt;

    /* Go Throught the Linked List */
    Node* cur = this->head->next;
    while(cur != this->tail){
        if(cur->host == host && cur->uri == uri){
            /* check for etag */
            int before   = cur->gr.writeTime;
            int timeDiff = tt.timeDifference(before, tt.currentTimeInSecond());
            gr = cur->gr;
            if(timeDiff < cur->gr.validTime){
                cs = CS_CACHE_VALID;
                break;
            }else{  /* needs revalidate */
                cs = CS_CACHE_NEED_REVALIDATE;
                break;
            }
        }

        cur = cur->next;
    }

    /********************************** critical section ends *******************************************************/

    P(&this->mutex);

    this->numReader--;
    /** last reader, release write restriction */
    if(this->numReader == 0){
        V(&this->W);
    }

    V(&this->mutex);

    return {cs, gr};
}

/**
 * Write the corresponding response to the cache.
 * We will acquire the Write Lock
 *
 * @param host the identifier for host
 * @param uri  the resource identifier for this uri this host
 * @param response the response we got from server
 *
 * @return whether we write it success or not.
 */
int Cache::writeToCache(string host, string uri, GetResponse gr){
    Log myLog;
    P(&this->W);

    /*********** critical section begins **************************************/
    int cacheSize = gr.response.size();
    if(cacheSize > MAX_OBJECT_SIZE || cacheSize + totalSize > MAX_CACHE_SIZE){
        myLog.write("too large, cannot write to cache " + host + "/" + uri);
    }else{
        Node* newNode = new Node(host, uri, gr);
        this->addNode(newNode);
        myLog.write(" done, have written to cache");
    }
    /*********** critical section ends ***************************************/

    V(&this->W);
    return 0;
}


/**
 * Format Cache and print in a nice way
 */
void Cache::printCache(){
    Log myLog;
    myLog.write("In cache, we have ");
    Node* cur = this->head->next;
    while(cur != this->tail){
        myLog.write("["+cur->host + cur->uri+"]" + ": " + cur->gr.response);
        cur = cur->next;
    }
}

/**
 * Add brand new response into cache
 *
 * @param nodeToAdd the response we add
 */
void Cache::addNode(Node* nodeToAdd){
    if(nodeToAdd == nullptr)
        return;

    nodeToAdd->next = this->head->next;
    nodeToAdd->prev = this->head;

    this->head->next->prev = nodeToAdd;
    this->head->next       = nodeToAdd;
}

/**
 * Out-date the cache, since we limit the size for the cache
 *
 * @param node the node we delete, usually out-of-date or exist longest
 */
void Cache::deleteNode(Node* node) {
    if(node == nullptr){
        return;
    }
    Log myLog;

    node->next->prev = node->prev;
    node->prev->next = node->next;
    myLog.write("NOTE evicted " + getFirstLine(node->gr.response) + " from cache");
    delete(node);
}
/**
 * Traversal the map, clear all invalid cache by specificing hostName and URI
 */
void Cache::clearAllInvalidCache(string hostName, string uri){

    P(&this->W);

    /*********** critical section begins **************************************/
    Node* cur = this->head->next;
    while(cur != this->tail){
        if(hostName == cur->host && uri == cur->uri){
            cur = cur->next;
            deleteNode(cur->prev);
        }else{
            cur = cur->next;
        }
    }
    /*********** critical section begins **************************************/

    V(&this->W);
}

