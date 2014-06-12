#ifndef __UNIX_WRAPPER_H__
#define __UNIX_WRAPPER_H__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LOG_DEBUG_STR 1
#if LOG_DEBUG_STR
  #define DebugStr(args...) fprintf(stderr, args);
#else
  #define DebugStr(args...)
#endif

// Error Handling
extern int h_errno;     // defined by BIND for DNS erros
// these functions will terminate the current process by calling exit(0)
void unix_error(char *msg);
void posix_error(int code, char *msg);
void dns_error(char *msg);
void app_error(char *msg);

// Process control wrappers
extern char **environ;  // defined by libc
pid_t Fork(void);
void Execve(const char* filename, char *const argv[], char *const envp[]);
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *iptr, int options);
void Kill(pid_t pid, int signum);
unsigned int Sleep(unsigned int secs);
void Pause(void);
unsigned int Alarm(unsigned int seconds);
void Setpgid(pid_t pid, pid_t pgid);
pid_t Getpgrp();

// Signal wrappers
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigaddset(sigset_t *set, int signum);
void Sigdelset(sigset_t *set, int signum);
int Sigismember(const sigset_t *set, int signum);

// Unix I/O Wrappers
int Open(const char *pathname, int flags, mode_t mode);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
off_t Lseek(int fildes, off_t offset, int whence);
void Close(int fd);
int Select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
           struct timeval *timeout);
int Dup2(int fd1, int fd2);
void Stat(const char *filename, struct stat *buf);
void Fstat(int fd, struct stat *buf);

// Memory mapping wrappers
void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
void Munmap(void *start, size_t length);



// RIO (Robust I/O)
// Unbuffered input and output functions for reading and writting
// binary data to and from network
// return value:
// on success return the actually bytes read, otherwise return -1
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);

// Buffered input and output functions
#define RIO_BUFFER 8192
typedef struct {
  int rio_fd;
  int rio_cnt;
  char *rio_bufptr;
  char rio_buf[RIO_BUFFER];
} rio_t;

// init the buffer
void rio_readinitb(rio_t *rp, int fd);
// read a line per call
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
// read n bytes per call
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);

#endif