#include "unix_wrapper.h"
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
// Error Handling
void unix_error(char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(0);
}

void posix_error(int code, char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(code));
  exit(0);
}

void dns_error(char *msg) {
  fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
  exit(0);
}

void app_error(char *msg) {
  fprintf(stderr, "%s\n", msg);
}

// RIO (Robust I/O)
// Unbuffered input and output functions for reading and writting
// binary data to and from network

// return value:
// on success return the actually bytes read, otherwise return -1
ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nread = read(fd, bufp, nleft)) < 0) {
      if (errno == EINTR) { // Interrupted by signal handler return
        nread = 0;
      } else {
        return -1; // error
      }
    } else if (nread == 0) { // EOF
      break;
    }

    nleft -= nread;
    bufp += nread;
  }
  return (n - nleft);
}

// return value:
// on success return the actually bytes write, otherwise return -1
ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nwritten;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nwritten = write(fd, bufp, nleft)) <= 0) {
      if (errno == EINTR) { // Interrupted by signal handler return
        nwritten = 0;
      } else {
        return -1; // error
      }
    }
    nleft -= nwritten;
    bufp += nwritten;
  }
  return n;
}

// return value:
// on success return the actaully bytes read, otherwise return -1
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
  int cnt;
  while (rp->rio_cnt <= 0) { // Refill if the buf is empty
    rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, RIO_BUFFER);

    if (rp->rio_cnt < 0) { // Interup
      if (errno != EINTR) return -1;
    } else if (rp->rio_cnt == 0) { // EOF
      return 0;
    } else {
      rp->rio_bufptr = rp->rio_buf;
    }
  }

  // copy data from internal buffer to user buffer
  cnt = n < rp->rio_cnt ? n : rp->rio_cnt;
  memcpy(usrbuf, rp->rio_bufptr, cnt);
  rp->rio_bufptr += cnt;
  rp->rio_cnt -= cnt;
  return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
  int n, rc;
  char c, *bufp = usrbuf;
  // copy at most maxlen - 1 bytes, the left 1 byte for null terminate ch
  for (n = 1; n < maxlen; ++n) {
    if ((rc = rio_read(rp, &c, 1)) == 1) {
      *bufp++ = c;
      if (c == '\n') {
        break;
      }
    } else if (rc == 0) {
      if (n == 1) return 0; // EOF, no data read
      else break;
    } else {
      return -1;
    }
  }
  *bufp = '\0';
  return n;
}

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
  size_t nleft = n;
  ssize_t nread;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nread = rio_read(rp, bufp, nleft)) < 0) {
      if (errno == EINTR) nread = 0;
      else return -1;
    } else if (nread == 0) {
      break;
    }
    nleft -= nread;
    bufp += nread;
  }
  return (n - nleft);
}