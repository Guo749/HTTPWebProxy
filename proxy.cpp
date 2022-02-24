#include "utilities.h"
#include "Rio.h"
#include "WorkBuffer.h"
#include "RequestHandler.h"
#include "Cache.h"
#include "GetHandler.h"
#include "PostHandler.h"
#include "ConnectHandler.h"

void handleRequest(ClientInfo info);
void* targetFunc(void* argvp);

/* buffer for worker threads */
SodaMachine sm(NUM_BUFFER);

/* in memory cache for storing response */
Cache cache;


/**
 * Signal Handler for Broken Pipe
 * the function is to prevent program from exiting
 *
 * @param sig Broken Pipe
 */
void signPipe_handler(int sig){
    static int myCount = 0;
    cout << "broken pipe signal is captured " << myCount++ << endl;
    return;
}

/**
 * Handing incoming request, Only support Get / Post / Connect
 *
 * @param info the client info
 */
void handleRequest(ClientInfo info){
    /* the argument used to parse the first line of http request */
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];

    /* client request header and body */
    char clientRequest[MAXLINE];

    /** step1: read request from client */
    Rio rio(info.fd);

    if(rio.Rio_readlineb(buf, MAXLINE) == 0){
        /** the request has the empty space, ignore */
        return;
    }

    sscanf(buf, "%s %s %s", method, uri, version);
    char firstLine[MAXLINE / 8];
    sprintf(firstLine, "%s %s %s", method, uri, version);
    info.request = string(buf);

    Log myLog;
    string temp = info.UUID + ": " + string(firstLine) + " FROM " + info.IPAddr + " @" + getFirstLine(info.arrivalTime);
    myLog.write(temp);

    /** step2: determine if it is a valid request */
    if(strcasecmp(method, "GET") == 0){
        GetHandler gh;
        gh.handleGet(rio, clientRequest, method, uri, version, cache, info);
    }else if(strcasecmp(method, "POST") == 0){
        PostHandler ph;
        ph.handlePost(rio, clientRequest, method, uri, version, info);
    }else if(strcasecmp(method, "CONNECT") == 0){
        ConnHandler ch;
        ch.handleConn(rio, clientRequest, method, uri, version, info);
    }else {/** other method, return not implemented */
        string res = formatErrorHTML("501", "Not Implemented", "HTTP/1.1");
        sendErrorHTML(rio, res, info);
    }
}


/**
 * @brief used for thread to run, each of thread grasp one fd and work with it
 * 
 * @param argvp: null
 * @return null
 */
void* targetFunc(void* argvp){
    pthread_detach(pthread_self()); //no need to wait for master thread

    while(1){
        ClientInfo info = sm.removeFd();
        handleRequest(info);
        close(info.fd);
    }
}

/** master thread begins */
int main(int argc, char** argv){
    Log myLog;
    if(argc != 2){
        cout << ("proxy usage: ./proxy <port number>") << endl;
        return EXIT_FAILURE;
    }

    if(signal(SIGPIPE, signPipe_handler) == SIG_ERR){
        cout << "Signal Handler Pipe Installment failed" << endl;
        return EXIT_FAILURE;
    }

    TimeTools tt;
    cout << tt.currentTime() << " now proxy is running on host: " + getHostName() + ":" + string(argv[1]) << endl;
    cout << "level of thread: "  << NUM_THREAD << endl;
    cout << "level of buffers: "  << NUM_BUFFER << endl;

    int listenfd = createListenFd(argv[1]);
    struct sockaddr_storage clientAddr;
    char hostName[MAXLINE], port[MAXLINE];

    /* pre-create thread workers */
    pthread_t tid;
    for(int i = 0; i < NUM_THREAD; i++){
        pthread_create(&tid, NULL, targetFunc, NULL);
    }

    int requestUUID = 0;
    while(1){
        socklen_t addrLen = sizeof(struct sockaddr_storage);
        int clientfd = Accept(listenfd, (SA*)&clientAddr, &addrLen);
        if(clientfd < 0){
            myLog.write("bad request from client, we discard");
            continue;
        }

        Getnameinfo((SA*)&clientAddr, addrLen, hostName, MAXLINE, port, MAXLINE, 0);
        //myLog.write("Accepting Connection from " +  string(hostName) + " "+ string(port));
        cout << "Accepting Connection from " +  string(hostName) + " "+ string(port) + "\n";

        ClientInfo clientInfo(to_string(requestUUID), hostName, clientfd, tt.currentTimeNoBracket());
        requestUUID++;
        sm.insertFd(clientInfo);
    }

}
