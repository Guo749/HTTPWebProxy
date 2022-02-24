#include "RequestHandler.h"


/**
 * Command tool for request GET/POST/CONN to deal with header
 * @return 1 if request is valid, 0 if not
 */
int processHeader(Rio &rio, char *clientRequest, char *method, char *uri, char *version, char *hostName, char *portNum) {
    char buf[MAXLINE];
    Log myLog;
    /* 1. add GET HOSTNAME HTTP/1.0 to header && Host Info */
    sprintf(clientRequest, "%s %s %s\n", method, uri, version);
    int hasSeenEnd = 0;
    while (rio.Rio_readlineb(buf, MAXLINE) > 0) {
        strcat(clientRequest, buf);

        /* end of the get request header */
        if (strcmp(buf, "\r\n") == 0) {
            hasSeenEnd = 1;
            break;
        }
    }

    if(hasSeenEnd == 0){
        return hasSeenEnd;
    }

    try {
        /** fill the field of host and port */
        setupHostAndPort(clientRequest, hostName, portNum, uri);
    }catch(invalid_argument& e){
        myLog.write("invalid request, discard it");
        return 0;
    }

    printRequest(clientRequest, 632);

    return hasSeenEnd;
}

/**
 * According to the client request, abstract the hostname and port number
 *
 * @param clientRequest the client request we examine
 * @param hostName host, like www.google.com
 * @param portNum  port number, like 80
 * @param uri universal resource identifier
 * @exception: it will throw invalid argument if it meets bad request
 */
void setupHostAndPort(char *clientRequest, char *hostName, char *portNum, char *uri) {
    char *findp = strcasestr(clientRequest, HOST_PREFIX);
    int hostNameIndex = 0;
    int portNumIndex = 0;

    /* no host specific, need to split it from uri */
    if (findp == NULL) {
        findp = strcasestr(uri, WEB_PREFIX);
        if(findp == NULL){
            throw invalid_argument("bad request");
        }

        findp += strlen(WEB_PREFIX);
        while (*findp != '/' && *findp != ' ' && *findp != ':' && *findp != '\r') {
            hostName[hostNameIndex++] = *findp;
            findp++;
        }
        hostName[hostNameIndex] = 0;

        if (*findp == ':') {
            findp++;
            while (isdigit(*findp)) {
                portNum[portNumIndex++] = *findp;
                findp++;
            }

            portNum[portNumIndex] = 0;
        } else {
            portNum[0] = '8';
            portNum[1] = '0';
            portNum[2] = 0;     //null terminator
        }

    } else { /* user specific the request in its request header, format-> Host: */
        // eg: www.example.com:80
        findp += strlen(HOST_PREFIX);

        while (*findp != '\n' && *findp != ':' && *findp != '\r') {
            hostName[hostNameIndex++] = *findp;
            findp++;
        }

        hostName[hostNameIndex] = 0;

        if (*findp == ':') {
            findp++;
            while (isdigit(*findp)) {
                portNum[portNumIndex++] = *findp;
                findp++;
            }

            portNum[portNumIndex] = 0;
        } else {
            portNum[0] = '8';
            portNum[1] = '0';
            portNum[2] = 0;     //null terminator
        }
    }

}

/**
 * When unexpected things happened, we use this to send back to client
 *
 * @param rio the client rio
 * @param html the message we are going to send
 */
void sendErrorHTML(Rio& rio, string html, ClientInfo info){
    int len = html.size();
    char arr[len + 1];
    strcpy(arr, html.c_str());
    rio.Rio_writen(arr, len);
    Log myLog;
    myLog.write(info.UUID + ": " + "Responding " + html);
}


/**
 * This is used to calculate the length of POST body
 * note that every POST field has extra \r\n, need to pay attention to this
 *
 * @param clientRequest the client request to abstract the content length
 * @return the content length for request body
 */
int getContentLength(char *clientRequest) {
    char *findContentField = strcasestr(clientRequest, CONTENT_LEN);

    /* invalid post request, return directly */
    if (findContentField == NULL) {
        return 0;
    }

    int res = 0;
    findContentField += strlen(CONTENT_LEN);
    while (isdigit(*findContentField)) {
        res *= 10;
        res += *findContentField - '0';
        findContentField++;
    }

    return res;
}

/**
 * Used to read and write chunked data (response)
 * can be used by get / post / connect
 *
 * @param rioRead the rio to read
 * @param rioWrite  the rio to write
 * @return data read from this process
 */
string readAndWriteChunkedData(Rio& rioRead, Rio& rioWrite){
    char buffer[MAXLINE];
    int n;
    string data;
    Log myLog;
    while( (n = rioRead.Rio_readlineb(buffer, MAXLINE)) > 0){
        rioWrite.Rio_writen(buffer, n);

        data += string(buffer);

        if(strcasestr(buffer, CHUNKED_SEPARATOR) != NULL){
            myLog.write("we have meet the end of chunked data");
            break;
        }
    }


    return data;
}

/**
 * read data using content length to terminate
 * if rioWrite.fd == -1, we do not need to write
 * @param rioRead
 * @param rioWrite
 * @param remains
 * @return
 */
void readAndWriteContentLengthData(Rio& rioRead, Rio& rioWrite, int remains, char* data){
    int n = 0;
    int temp = remains;
    while( (n = rioRead.Rio_readnb(data, remains)) > 0){

        if(rioWrite.rio_fd > 0) {
            rioWrite.Rio_writen(data, n);
        }

        remains -= n;
        if(remains <= 0){
            break;
        }
    }

    data[temp] = 0; // null terminator
}

void printRequest(char* clientRequest, int lineNumber){
    printf("=== request begins %d===\n", lineNumber);
    printf("%s\n", clientRequest);
    printf("=== request ends ===\n");
}

/**
 * Send bad request response to the client
 *
 * @param rio client file descriptor
 * @param info client info
 */
void sendBadRequest(Rio& rio, ClientInfo info){
    Log myLog;
    string errorHTML = formatErrorHTML("400", "Bad Request", "HTTP/1.1");
    sendErrorHTML(rio, errorHTML, info);
    myLog.write(info.UUID + ": " + "bad request");
}

/**
 * Send Gate way timeout to client
 */
void sendGatewayTimeout(Rio& rio, ClientInfo info, char* hostName, char* portNum){
    Log myLog;
    myLog.write("bad hostname and port num " + string(hostName) + " " + string(portNum));
    string errorHTML = formatErrorHTML("504", "Gateway Timeout", "HTTP/1.1");
    sendErrorHTML(rio, errorHTML, info);
}