#ifndef PROXY_RIO_H
#define PROXY_RIO_H

#include "utilities.h"

/**
 * @brief Robust IO Package, used for 
 *        reading and writing in the network
 *        because there are some issues using standard UNIX IO.
 *        Thus, this is a wrapper for that
 */
class Rio{
public:
    /** the file descriptor it is working on */
    int rio_fd;

    /** how many bytes we still left */
    size_t rio_cnt;

    /** the pointer point at the current location of the buffer */
    char* rio_bufptr;

    /** buf we store all bytes from packages sent to us*/
    char rio_buf[MAXLINE];

    Rio(int fd);
    ssize_t Rio_readlineb(void* buf, size_t maxlen);
    ssize_t Rio_writen(void* usrbuf, size_t n);
    ssize_t rio_read(char* usfbuf, size_t n);
    ssize_t Rio_readnb(char* usrbuf, size_t);
};


#endif