
#include "PostHandler.h"


/**
 * Post handler
 *
 * @param rio rio responsible for client side
 * @param clientRequest the client request
 * @param method CONNECT
 * @param uri
 * @param version normally http/1.1
 * @param info client info, including UUID
 */
void PostHandler::handlePost(Rio &rio, char *clientRequest,
                             char *method, char *uri, char *version, ClientInfo info) {
    Log myLog;
    char hostName[100];
    char portNum[100];

    int rv = processHeader(rio, clientRequest, method, uri, version, hostName, portNum);
    if(rv == 0){
        sendBadRequest(rio, info);
        return;
    }

    /* indicates how many bytes we read for post body */
    int isChunked = checkChunked(clientRequest);
    int remains   = getContentLength(clientRequest);
    if((isChunked == 0 && remains == 0) || (isChunked == 1 && remains != 0)){
        sendBadRequest(rio, info);
        return;
    }

    int clientfd = createClientFd(hostName, portNum);
    if (clientfd == -1) {
        sendGatewayTimeout(rio, info, hostName, portNum);
        return;
    }

    Rio fakeRio(-1);
    if(isChunked == 1){
        string dataStr = readAndWriteChunkedData(rio, fakeRio);
        int len = dataStr.size() + 1;
        char data[len + 1];
        strcpy(data, dataStr.c_str());
        data[len] = 0;

        strcat(clientRequest, data);
        handleResponse(rio, clientfd, clientRequest, hostName, info);
    }else{ //content length = a number
        char data[remains + MINLINE];
        readAndWriteContentLengthData(rio, fakeRio, remains, data);

        strcat(clientRequest, data);
        handleResponse(rio, clientfd, clientRequest, hostName, info);

    }
}

/**
 * Check if data is transmitted as chunked data
 *
 * @param clientRequest to check
 * @return 1 true, 0 false
 */
int PostHandler::checkChunked(char* clientRequest){
    char* findp = strcasestr(clientRequest, TRANSDER_CHUNKED);
    if(findp == NULL){
        return 0;
    }else{
        return 1;
    }
}

/**
 * Handle Post Response
 */
void PostHandler::handleResponse(Rio &rio, int clientfd, char *clientRequest, char *hostName, ClientInfo info) {
    Log myLog;
    Rio rioServer(clientfd);

    rioServer.Rio_writen(clientRequest, strlen(clientRequest));
    myLog.write(info.UUID + ": " + "Requesting " + info.request + " from " + hostName);


    char response[MAXLINE];
    int n;
    char serverResponse[MAXLINE];
    memset(serverResponse, 0, MAXLINE);

    while ((n = rioServer.Rio_readlineb(response, MAXLINE)) > 0) {
        strcat(serverResponse, response);
        rio.Rio_writen(response, n);
        printf("%s", response);
        if (strcmp(response, SEPARATOR_MACRO) == 0) {
            /* header ends */
            break;
        }
    }

    int remains = getContentLength(serverResponse);

    char data[remains + MINLINE];
    readAndWriteContentLengthData(rioServer, rio, remains, data);

    myLog.write(info.UUID + ": " + "Tunnel closed");
    myLog.write(info.UUID + ": " + "Finish Post Request. Now serve another client");

    cout << "finish this request, return " << endl;
}
