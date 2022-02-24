#include "Rio.h"

Rio::Rio(int fd) : rio_fd(fd), rio_cnt(0), rio_bufptr(rio_buf) {
}

/**
 * @brief Buffered function, use this to read one line by one line
 *        this can be used in parsing http request header & body
 * 
 *        [caution]: cannot use this to determine the end of line in binary file
 *        like image or execute file
 * @param usrbuf 
 * @param maxlen 
 * @return ssize_t 
 */
ssize_t Rio::Rio_readlineb(void* usrbuf, size_t maxlen){
    size_t n, rc;
    char c;
    char* bufp = (char*)usrbuf;

    for (n = 1; n < maxlen; n++) {
        if ((rc = rio_read(&c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                n++;
                break;
            }
        } else if (rc == 0) {
            if (n == 1)
                return 0;
            else
                break;
        } else
            return -1;
    }

    /* char terminator */
    *bufp = 0;
    return n - 1;    
}



/**
 * @brief write to the file descriptor
 *        this function should not report error hopefully
 * 
 * @param usrbuf 
 * @param n 
 * @return ssize_t 
 */
ssize_t Rio::Rio_writen(void* usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char*)usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(this->rio_fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  
                nwritten = 0;
            else
                return -1;       
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;    
}



/**
 * @brief Unix Wrapper for standard read
 * 
 * @param usrbuf 
 * @param n 
 * @return ssize_t 
 */
ssize_t Rio::rio_read(char *usrbuf, size_t n) {
    int cnt;

    while (this->rio_cnt <= 0) {  
        this->rio_cnt = read(this->rio_fd, (void*) this->rio_buf,
                           sizeof(this->rio_buf));
        if (this->rio_cnt < 0) {
            if (errno != EINTR) /* Interrupted by sig handler return */
                return -1;
        } else if (this->rio_cnt == 0)  /* EOF */
            return 0;
        else
            this->rio_bufptr = this->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n < this->rio_cnt ? n : this->rio_cnt;
    memcpy(usrbuf, this->rio_bufptr, cnt);
    this->rio_bufptr += cnt;
    this->rio_cnt -= cnt;
    
    return cnt;
}

ssize_t Rio::Rio_readnb(char* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = rio_read(bufp, nleft)) < 0)
            return -1;
        else if (nread == 0)
            break;
        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}