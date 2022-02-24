#include "GetHandler.h"

/**
 * Get Handler, used to deal with get request
 * if the criteria match, we will store it into the cache according to different policy
 */
void GetHandler::handleGet(Rio &rio, char *clientRequest, char *method, char *uri, char *version, Cache &cache,
                           ClientInfo info) {
    Log myLog;
    /** connect with the backup */
    char hostName[100];
    char portNum[100];

    int rv = processHeader(rio, clientRequest, method, uri, version, hostName, portNum);
    if(rv == 0){
        string res = formatErrorHTML("400", "Bad Request", "HTTP/1.1");
        sendErrorHTML(rio, res, info);
        cout << "send back to client " << res;
        return;
    }

    /** check if cache has one */
    pair<CacheStatus, GetResponse> p = cache.ReadFromCache(string(hostName), string(uri), info);
    cout << "get cache policy from reading cache " << endl;
    handleResponse(p, rio, info, hostName, portNum, clientRequest, uri, cache);
    cout << "*************done*************" << endl;
}

/**
 *
 * @param info
 * @param clientRequest
 * @param etag
 */
string GetHandler::getETagIfAny(string response){
    string res;
    char needle[6] = "etag:";

    int len = response.size();
    char arr[len + 1];
    strcpy(arr, response.c_str());
    char* findp = strcasestr(arr, needle);

    if(findp != NULL){
        while(*findp != ':'){
            findp++;
        }

        findp++;

        while(*findp != '\r' && *findp != '\n'){
            res.push_back(*findp);
            findp++;
        }
    }

    return res;
}

/**
 * Format good response to client
 * as well as dealing with cache
 */
void GetHandler::handleResponse(pair<CacheStatus, GetResponse> p, Rio &rio, ClientInfo &info,
                                char *hostName, char *portNum, char *clientRequest, char *uri,
                                Cache &cache) {
    Log myLog;
    CacheStatus cs = p.first;
    GetResponse gr = p.second;

    if (cs == CS_CACHE_VALID) {
        sendCacheResponse(rio, gr, info);
        return;
    } else if (cs == CS_CACHE_NOT_FOUND) {
        myLog.write(info.UUID + ": " + "not found in cache");
    } else if (cs == CS_CACHE_NEED_REVALIDATE) {
        myLog.write(info.UUID + ": " + "NOTE Cache needs to re-validate ETAG");
        myLog.write(info.UUID + ": " + "gr has " + gr.eTag);
        int rv = handleETag(clientRequest, info, hostName, portNum, gr);
        myLog.write(info.UUID + ": " + "value is " + to_string(rv));
        if(rv == 1){
            sendCacheResponse(rio, gr, info);
            return;
        }else{
            cache.clearAllInvalidCache(string(hostName), string(uri));
        }
    }else {
        throw invalid_argument(" cache status is invalid!");
    }

    /* establish connection with remote server */
    int clientfd = createClientFd(hostName, portNum);

    if(clientfd < 0){
        myLog.write(info.UUID + ": " + "invalid hostname "+ string(hostName) + "or portNum" + string(portNum));
        myLog.write(info.UUID + ": " + "we cannot find the host, do that again. we return");
        string errorHTML = formatErrorHTML("504", "Gateway Timeout", "HTTP/1.1");
        sendErrorHTML(rio, errorHTML, info);
        return;
    }

    Rio rioServer(clientfd);
    cout << "establish connection" << endl;
    /** send the request to the remote server */
    rioServer.Rio_writen(clientRequest, strlen(clientRequest));
    myLog.write(info.UUID + ": " + "Requesting " + getFirstLine(info.request) + " from " + getFirstLine(hostName));

    /** step4: read the response from server and send it to the client */
    char tinyResponse[MAXLINE];
    string response = "";
    int cacheTime = 0;
    CachePolicy cp;       //default: no cache stored.

    int isChunked = 0;
    int contentLength = 0;
    cp = handleResponseHeader(rio, rioServer, response, tinyResponse, &cacheTime, &isChunked, &contentLength);

    if(cp == CP_BAD_REQUEST_DISCARD){
        sendBadRequest(rio, info);
        return;
    }

    handleResponseBody(rio, rioServer, tinyResponse, response, &isChunked, contentLength);

    myLog.write(info.UUID + ": " + "Received " + getFirstLine(response) + "from " + string(hostName));
    myLog.write(info.UUID + ": " + "Response: " + getFirstLine(gr.response));
    myLog.write(info.UUID + ": " + "Tunnel closed");
    handleCache(info, response, cp, cacheTime, cache, hostName, uri);
    cout << "done, we do next one" << endl;
}

/**
 * This is used to check with remote server if the cache is still valid by sending
 * etag and see if match
 *
 * @return the cache is valid or not, 1 is valid, 0 is not valid cache
 */
int GetHandler::handleETag(char* clientRequest, ClientInfo info, char* hostName, char* portNum, GetResponse gr){
    Log myLog;

    /* add to client request if it does not have etag or if non match */
    if(gr.eTag != "") {
        char* findp = strcasestr(clientRequest, "ETag: ");
        char* findq = strcasestr(clientRequest, "If-None-Match: ");

        if(findp == NULL && findq == NULL){
            char temp[MINLINE];
            char* index = temp;
            sprintf(temp, "If-None-Match: \"%s\"\r\n", gr.eTag.c_str());
            char* findSep = strstr(clientRequest, "\n\r\n");
            findSep += 1;
            while(*index != '\n'){
                *findSep = *index;
                findSep++;
                index++;
            }

            *findSep = *index;
            strcat(findSep, SEPARATOR_MACRO);

            cout << "after concatenate" << endl;
            cout << string(clientRequest) << endl;
        }
    }


    int clientfd = createClientFd(hostName, portNum);
    if(clientfd < 0){
        myLog.write(info.UUID + ": " + "cannot connect to the remote server");
        return 0;
    }

    Rio rioServer(clientfd);
    rioServer.Rio_writen(clientRequest, strlen(clientRequest));
    char tinyResponse[MAXLINE];
    int n;
    int firstLine  = 1;
    int validCache = 0;
    while((n = rioServer.Rio_readlineb(tinyResponse, MAXLINE)) > 0){
        if(firstLine == 1){
            if(strcasestr(tinyResponse, "304") != NULL){
                validCache = 1;
                myLog.write(info.UUID + ": " + "verified by ETag, still valid");
                break;
            }
            firstLine = 0;
        }else{ // we do not care what remains

        }
    }

    close(clientfd);
    return validCache;
}

/**
 * Two senarios will call this method.
 *  1. when cache is valid under certain key words, like cache is still valid
 *  2. after confirm with remote server, ETag has been the same
 *
 * @param rio client file descriptor
 * @param response the response for client
 * @param info client information
 */
void GetHandler::sendCacheResponse(Rio& rio, GetResponse gr, ClientInfo info){
    Log myLog;
    int len = gr.response.length();
    char arr[len + 1];
    strcpy(arr, gr.response.c_str());
    printRequest(arr, 209);
    rio.Rio_writen(arr, len);
    myLog.write(info.UUID + ": " + "in cache, valid");
    myLog.write(info.UUID + ": " + "Response: " + getFirstLine(gr.response));
}

/**
 * Handle Response header, also check if it is a valid header
 * i.e. Content-length and Transfer-data: chunked cannot exist at the same time
 *
  * @return cache policy from header
 */
CachePolicy GetHandler::handleResponseHeader(Rio& rio, Rio& rioServer, string& response,
                                             char* tinyResponse, int* cacheTime, int* isChunked, int* contentLength){
    /* deal with header */
    int n;
    CachePolicy cp = CP_NOT_FOUND;
    while ((n = rioServer.Rio_readlineb(tinyResponse, MAXLINE)) > 0) {
        response += string(tinyResponse);
        rio.Rio_writen(tinyResponse, strlen(tinyResponse));

        if (strcasestr(tinyResponse, CACHE_CONTROL_FLAG) != NULL) {
            cp = getCacheControlTime(tinyResponse, cacheTime);
        }

        if (strcasestr(tinyResponse, TRANSDER_CHUNKED) != NULL) {
            *isChunked = 1;
        }

        if(strcasestr(tinyResponse, CONTENT_LEN) != NULL){
            *contentLength = getContentLength(tinyResponse);
        }

        if (strcmp(tinyResponse, SEPARATOR_MACRO) == 0) {
            break; //we meet separator
        }
    }


    if(*isChunked && *contentLength != 0){
        return CP_BAD_REQUEST_DISCARD;
    }

    return cp;
}

/**
 * Handle Response body by two different ways
 *  1. using chunked, it will be ended by 0\r\n\r\n
 *  2. using content length, it will be ended when we read all remains
 */
void GetHandler::handleResponseBody(Rio& rio, Rio& rioServer, char* tinyResponse,
                                    string& response, int* isChunked, int contentLength) {
    /* deal with body */
    if(*isChunked == 1){
        string data = readAndWriteChunkedData(rioServer, rio);
        response += data;
    }else{
        char data[contentLength + MINLINE];
        readAndWriteContentLengthData(rioServer, rio, contentLength, data);
        response += string(data);
    }
}

/**
 * Based on cache policy, we deal response correspondingly
 */
void GetHandler::handleCache(ClientInfo info, string response, CachePolicy cp,
                             int cacheTime, Cache& cache, char* hostName, char* uri) {
    Log myLog;
    myLog.write(info.UUID + ": " + "sent the response to client, return");
    /* no need to cache into that */
    if (cp == CP_NO_STORE || cp == CP_MAX_AGE_ZERO || cp == CP_PRIVATE
        || cp == CP_NO_CACHE_POLICY || cp == CP_NOT_FOUND
        || (cp == CP_CACHE_TIME_DEFINED && cacheTime == 0)) {
        string reason = (cp == CP_NO_STORE  ? "not to store cache"                  :
                         cp == CP_MAX_AGE_ZERO    ? "max age equals 0"                    :
                         cp == CP_PRIVATE    ? "cache needs to stay private"         :
                         cp == CP_NOT_FOUND ? "no cache option is on"               :
                         cp == CP_CACHE_TIME_DEFINED ? "max age = 0, invalid immediately" :
                         "no instruction for how to deal cache in response header");

        myLog.write(info.UUID + ": " + "not cacheable because " + reason);
        return;
    } else {
        if (cp == CP_NEVER_EXPIRED) {
            myLog.write(info.UUID + ": " + "cache never expires");
            cacheTime = YEAR_IN_SECOND;
        } else if(cp == CP_CACHE_TIME_DEFINED) { //cp == CACHE_TIME_DEFINED
            myLog.write(info.UUID + ": " + "cache expires in " + to_string(cacheTime));
        }

        TimeTools tt;
        GetResponse gr(cacheTime, tt.currentTimeInSecond(), response);
        gr.eTag = getETagIfAny(response);
        cache.writeToCache(hostName, uri, gr);
    }
}


/**
 * check if current get response can be cached or not
 * several rules to check, they are all in Cache-control
 *
 *  1. must-revalidate
 *  2. no store
 *  3. no cache
 *  4. max age = 0
 *
 *  all 4 situations can cause a response not to be cached
 *
 * @param findp header for Cache-control: max-age=60488, must-revalidate
 *
 * @return the duration we cache this response.
 */
CachePolicy GetHandler::getCacheControlTime(char *findp, int *age) {
    char* needRevalidate = strcasestr(findp, MUST_REVALIDATE);
    char* cacheNoStore   = strcasestr(findp, NO_STORE);
    char* noCache        = strcasestr(findp, NO_CACHE);
    char* privateCache   = strcasestr(findp, CACHE_PRIVATE);

    if (needRevalidate != NULL || cacheNoStore != NULL || noCache != NULL || privateCache != NULL) {
        if (needRevalidate != NULL || noCache != NULL)
            return CP_NEED_REVALIDATE;
        else if (cacheNoStore != NULL)
            return CP_NO_STORE;
        else
            return CP_PRIVATE;
    }

    char *IMMU = strcasestr(findp, CACHE_IMMU);

    /* this is a static resource, should cache forever, we will cache a year */
    if (IMMU != NULL) {
        return CP_NEVER_EXPIRED;
    }

    char *maxAge = strcasestr(findp, MAX_AGE);
    if (maxAge != NULL) {
        *age = 0;

        maxAge += strlen(MAX_AGE);
        while (isdigit(*maxAge)) {
            *age *= 10;
            *age += *maxAge - '0';
            maxAge++;
        }

        return CP_CACHE_TIME_DEFINED;
    } else {
        /* no cache policy found, we return 0 */
        return CP_NOT_FOUND;
    }
}

