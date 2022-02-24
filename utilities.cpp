#include "utilities.h"

int Log::hasInitialized = 0;
sem_t Log::WriteLock;
ofstream Log::out;

/**
 * Only initialize once, three steps
 * 1. clear all possible old log records
 * 2. create new one
 * 3. initialize write_lock and out, to make sure at one time only one thread
 *    can access to that
 */
Log::Log() {
    if(hasInitialized == 1){
        return;
    }

    TimeTools tt;
    /* step1: remove and open the log file */
    string temp = ("rm -rf " + fileLocation);
    const char* removeCommand = temp.c_str();
    cout << tt.currentTime() << " remove the log file if possible "<< endl;
    system(removeCommand);

    /* step2: create a new log file */
    string temp2 = "mkdir -p /var/log/erss && touch " + fileLocation;
    const char* createCommand = temp2.c_str();
    cout << tt.currentTime() << " creating a brand new log file in " + fileLocation << endl;
    system(createCommand);

    /* step3: open it */
    out.open(fileLocation);

    /** assert file must be open */
    assert(out.is_open());

    /* only one thread can write it */
    sem_init(&WriteLock, 0, 1);
    hasInitialized = 1;

    cout << tt.currentTime() << " finish log initialization" << endl;
}


/**
 * Write into the log file
 *
 * @param msg the message we want to convey
 */
void Log::write(string msg) {
    TimeTools tt;

    P(&WriteLock);

    /************* critical section begins **************/
    out << tt.currentTime() << msg << " " << endl;
    /************* critical section ends   **************/

    V(&WriteLock);
}

/**
 * @brief initialize the server structure
 * 
 * @param port the port the server is providing the service on
 * @return int the listen fd for server
 */
int createListenFd(char* port){

    struct addrinfo hints, *listp;
    struct addrinfo *cur;
    int listenfd, rc;
    int timeout = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

    Log myLog;

    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
        myLog.write("Getaddrinfo Failed, port " + string(port) + "\n");
    }

    for (cur = listp; cur != NULL; cur = cur->ai_next) {
        if ((listenfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
            continue;  

        /* reuse that socket address quickly */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &timeout, sizeof(int));

        if (bind(listenfd, cur->ai_addr, cur->ai_addrlen) == 0){
            break;
        }

        if (close(listenfd) < 0) { 
            myLog.write("close file descriptor in utilities.cpp");
        }
    }

    freeaddrinfo(listp);

    /* no address is available */
    if (cur == NULL) {
        return -1;
    }
    
    if (listen(listenfd, BACKLOG) < 0) {
        close(listenfd);
        return -1;
    }
    return listenfd;

}

/**
 * @brief Unix Wrapper for accept, used to check the return value
 * 
 * @param fd file descriptor for client 
 * @param addr addr for client
 * @param addrlen 
 * @return int success / fail
 */
int Accept(int fd, sockaddr *addr, socklen_t *addrlen){
    Log myLog;

    int res;
    if((res = accept(fd, addr, addrlen)) < 0){
        myLog.write("Accept Fails");
    }

    return res;
}

/**
 * @brief Get name info wrapper given a socket addr
 *
 */
void Getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, size_t hostlen, char *serv, size_t servlen, int flags){
    Log myLog;
    int res;

    if ((res = getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags)) != 0){
        myLog.write("get name info error");
    }
}

/**
 * Create connection as a client.
 *
 * @param hostname the server we want to connect
 * @param port the port number we want to connect
 * @return
 */
int createClientFd(char* hostname, char* port){
    int clientfd, rc;
    struct addrinfo hints, *listp;
    struct addrinfo *cur;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;

    Log myLog;

    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0) {
        myLog.write("In ClientFD, getaddrinfo failed");
    }

    for (cur = listp; cur != NULL; cur = cur->ai_next) {
        if ((clientfd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol)) < 0)
            continue;

        if (connect(clientfd, cur->ai_addr, cur->ai_addrlen) != -1)
            break;
        if (close(clientfd) < 0) {
            myLog.write("close failed");
        }
    }

    freeaddrinfo(listp);

    if (cur == NULL){
        myLog.write("cannot find usable address, try again");
        return -1;
    }else{
        return clientfd;
    }
}


/**
 * @brief Wrapper for sem_wait, to acquire the lock
 *
 * @param sem
 */
void P(sem_t* sem){
    Log myLog;
    if(sem_wait(sem) < 0){
        myLog.write("In P, lock acquire error");
    }
}

/**
 * @brief Wrapper for sem_post, to release the lock
 *
 * @param sem
 */
void V(sem_t* sem){
    Log myLog;
    if(sem_post(sem) < 0){
        myLog.write("In V, lock release error");
    }
}

/**
 * This function will generate a 6-digit random UUID as request identifier
 *
 * @return
 */
string generateUUID(){
    string res;
    //format MMM-YYYY
    for(int i = 0; i < 3; i++){
        int randNum = rand() % 26;
        char ch = 'A' + randNum;
        res.push_back(ch);
    }

    for(int i = 0; i < 4; i++){
        int randNum = rand() % 10;
        res += to_string(randNum);
    }

    return res;
}

/**
 * Generate current time, format is like [Wed Feb  9 19:24:17 2022]
 *
 * @return string format of current time
 */
string TimeTools::currentTime(){

    time_t current_time;
    struct tm* local_time;
    current_time = time(NULL);
    local_time = localtime(&current_time);

    string res;
    res.push_back('[');
    res += string(asctime(local_time));
    res[res.size() - 1] = ']';

    return string(res);
}

string TimeTools::currentTimeNoBracket() {
    time_t current_time;
    struct tm* local_time;
    current_time = time(NULL);
    local_time = localtime(&current_time);

    string res;
    res += string(asctime(local_time));

    return string(res);
}

/**
 * Get current time in second format
 * used to calculate the cache validity
 *
 * @return current time in second, like  1644676687
 */
int TimeTools::currentTimeInSecond(){
    time_t seconds = time(NULL);
    return seconds;
}

/**
 * Get the time difference in seconds
 *
 * @param before the earlier time
 * @param after  the later time
 * @return the time difference in seconds
 */
int TimeTools::timeDifference(int before, int after) {
    if(before > after) {
        /* wrong argument, before should be smalled than after */
        return 0;
    }

    return after - before;
}

/**
 * Return the Error HTML Page
 *
 * @param type the error code, like 404, 500
 * @param reason why it fails, eg: Not Found, Not Implemented
 * @param host the host of server
 * @param uri the resource it request
 * @param version HTTP request version
 * @return a string format of error response in HTML style
 */
string formatErrorHTML(string type, string reason, string version){
    string res;

    /** attach response header */
    // eg: HTTP/1.1 404 NOT FOUND
    res += version + " " + type + " " + reason + NEWLINE;

    // eg: Server: Apache
    res += "Server: Apache"                    + NEWLINE;

    // eg: Connection: close
    res += "Connection: close"                 + NEWLINE;


    // attach  \r\n to be separator
    res +=                                        SEPARATOR;

    /** attach response body */

    res += "<!DOCTYPE html>"                               + NEWLINE;
    res += "<!-- In God, We Trust -->"                     + NEWLINE;
    res += "<html><head>"                                  + NEWLINE;
    res += "<title>Error, We do not implement it</title>"  + NEWLINE;
    res += "</head><body>"                                 + NEWLINE;
    res += "<h1>"+ reason+"</h1>"                          + NEWLINE;
    res += "</body></html>"                                + NEWLINE;
    res += SEPARATOR;
    return res;
}


/**
 * Get Current Host name, like vcm-23006.vm.duke.edu
 *
 * @return
 */
string getHostName(){
    char hostName[1024];
    hostName[1023] = 0;
    gethostname(hostName, 1023);
    return string(hostName);
}

/**
 * String decrator, used to trim the \n
 *  like msg = abc\r\n\def
 *              abc
 *              def
 *         we will only return abc
 * @param msg the message we are going to process
 * @return first line of code
 */
string getFirstLine(string msg){
    string res;
    int size = msg.size();
    for(int i = 0; i < size; i++){
        if(msg[i] == '\r' || msg[i] == '\n'){
            break;
        }

        res.push_back(msg[i]);
    }

    return res;
}
