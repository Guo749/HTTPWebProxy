#ifndef PROXY_UTILITIES_H
#define PROXY_UTILITIES_H

#include <memory.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <semaphore.h>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <ostream>
#include <assert.h>
#include <signal.h>

typedef struct sockaddr SA;


/**************** Setup For Socket begin *************************/
#define BACKLOG                 100
#define MAXLINE                 8192
#define MINLINE                 1024
#define NUM_BUFFER              20
#define NUM_THREAD              20
#define CHUNK_DATA_MAX          65536
/**************** Setup For Socket ends *************************/



/**************** request header begins *************************/
#define CONNECTION_ALIVE             "Connection: keep-alive"
#define CONTENT_LEN                  "Content-Length: "

#define END_OF_HTML                  "</html>"
#define WEB_PREFIX                   "http://"
#define HOST_PREFIX                  "Host: "
#define SEPARATOR_MACRO              "\r\n"
#define CHUNKED_SEPARATOR            "0\r\n\r\n"
#define TRANSDER_CHUNKED             "Transfer-Encoding: chunked"

const std::string PROXY_CONN_CLOSE = "Proxy-Connection: close\n";
const std::string CONN_CLOSE       = "Connection: close\n";
const std::string NEWLINE          = "\n";
const std::string SEPARATOR        = "\r\n";

#define CACHE_CONTROL_FLAG           "Cache-Control:"
#define MUST_REVALIDATE              "must-revalidate"
#define NO_STORE                     "no-store"
#define NO_CACHE                     "no-cache"
#define MAX_AGE                      "max-age="
#define CACHE_IMMU                   "immutable"
#define CACHE_PRIVATE                "Cache-Control: private"
#define CACHE_ETAG                   "ETag: "
#define YEAR_IN_SECOND               31536000
#define MAX_OBJECT_SIZE              102400  //cache max size
#define MAX_CACHE_SIZE               1049000 // one single response max size
/**************** request header ends *************************/

using namespace std;

/** helper function that will use across the class */
int createListenFd(char* );

int createClientFd(char* , char* );

int Accept(int , sockaddr *, socklen_t *);

void Getnameinfo(const struct sockaddr *, socklen_t , char *,
                 size_t , char *, size_t , int );

void P(sem_t* sem);

void V(sem_t* sem);

string formatErrorHTML(string type, string reason, string version);

string generateUUID();

string getHostName();

string getFirstLine(string);

//hex converter
int getIndexOfSigns(char);
long hexToDec(char *source);


class Log{
public:
    Log();
    void write(string msg);

private:
    const string fileLocation = "/var/log/erss/proxy.log";
    static ofstream out;
    static sem_t WriteLock;
    static int hasInitialized;
};

class TimeTools{
public:
    string currentTime();
    string currentTimeNoBracket();

    int currentTimeInSecond();

    int timeDifference(int, int);
};

/**
 * Used for indicating status when reading from cache
 */
enum CacheStatus {
    CS_CACHE_VALID,
    CS_CACHE_NOT_FOUND,
    CS_CACHE_NEED_REVALIDATE,
};

/**
 * Used for server response
 */
enum CachePolicy {
    /* do not store in proxy */
    CP_NO_STORE,
    CP_MAX_AGE_ZERO,

    /* need revalidate */
    CP_NEED_REVALIDATE,
    CP_CACHE_TIME_DEFINED,

    /* need fetch from server */
    CP_NOT_FOUND,
    CP_PRIVATE,
    CP_NO_CACHE_POLICY,

    /* use directly */
    CP_NEVER_EXPIRED,

    /* cannot recognize the policy for this response */
    CP_BAD_REQUEST_DISCARD,
};

#endif