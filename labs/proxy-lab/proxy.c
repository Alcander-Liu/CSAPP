/*
    Web Server <----> Log Proxy <----> Broswer
    Log Format: Date: browsdrIP URL size
    Part One:
        1. Create Server Socket Listen on a port
        2. Get the broswer reqeust
            (1) Parse Request
                <1> hostname
                <2> path-name
                <3> host port
            (2) hostname -> host IP
            (3) create socket (host IP, port)
            (4) Forward Request using the created socket
        3. Wait for the Server request
            (1) Parse Response and get the necessary info for logging
 */
#include "xnix_helper.h"
#include <stdarg.h>
#include <assert.h>
#define MAXLINE 8192
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

typedef struct {
  char *uri;
  char *host;
  char *port;
}HTTPRequest;

typedef struct {
  char *status;
  size_t size;
}HTTPResponse;

int HTTPRequestParser(const char *buffer, size_t size, HTTPRequest *request);
int HTTPResponseParser(const char *buffer, size_t size, HTTPResponse *response);
void FreeHTTPRequest(HTTPRequest *ptr);
void FreeHTTPREsponse(HTTPResponse *ptr);
void DispHTTPRequestStruct(const HTTPRequest *request);

char *GetBroswerRequest(int sock_fd, size_t *rec_size);

int main(int argc, char **argv) {
  /* Check arguments */
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
  }

  int server_fd = CreateServerSocket(argv[1], AF_INET, 10);
  if (server_fd == -1) {
    exit(0);
  }
  char client_addr[120];
  int broswer_fd = Accept(server_fd, -1, 0, client_addr);
  if (broswer_fd > 0) {
    size_t size = 0;
    char *request_buf = GetBroswerRequest(broswer_fd, &size);
    assert(request_buf);
    HTTPRequest request;
    HTTPRequestParser(request_buf, size, &request);
    DispHTTPRequestStruct(&request);
    Free(request_buf);
  }
  Close(broswer_fd);
  Close(server_fd);
  exit(0);
}


/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port) {
  char *hostbegin;
  char *hostend;
  char *pathbegin;
  int len;

  if (strncasecmp(uri, "http://", 7) != 0) {
    hostname[0] = '\0';
	return -1;
  }

  /* Extract the host name */
  hostbegin = uri + 7;
  hostend = strpbrk(hostbegin, " :/\r\n\0");
  len = hostend - hostbegin;
  strncpy(hostname, hostbegin, len);
  hostname[len] = '\0';

  /* Extract the port number */
  *port = 80; /* default */
  if (*hostend == ':')
  *port = atoi(hostend + 1);

  /* Extract the path */
  pathbegin = strchr(hostbegin, '/');
  if (pathbegin == NULL) {
    pathname[0] = '\0';
  } else {
    pathbegin++;
	strcpy(pathname, pathbegin);
  }
  return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
		              char *uri, int size) {
  time_t now;
  char time_str[MAXLINE];
  unsigned long host;
  unsigned char a, b, c, d;

  /* Get a formatted time string */
  now = time(NULL);
  strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

  /*
   * Convert the IP address in network byte order to dotted decimal
   * form. Note that we could have used inet_ntoa, but chose not to
   * because inet_ntoa is a Class 3 thread unsafe function that
   * returns a pointer to a static variable (Ch 13, CS:APP).
   */
  host = ntohl(sockaddr->sin_addr.s_addr);
  a = host >> 24;
  b = (host >> 16) & 0xff;
  c = (host >> 8) & 0xff;
  d = host & 0xff;

  /* Return the formatted log entry string */
  sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}

int HTTPRequestParser(const char *buffer, size_t size, HTTPRequest *request) {
  if (!strstr(buffer, "GET")) {
    app_error("Currently only support GET Method\n");
    return 0;
  }

  const char *uri_start = buffer + 4;
  const char *uri_end = strchr(uri_start, ' ');
  if (!uri_end) {
    app_error("Parse uri error\n");
    return 0;
  }

  size_t uri_size= uri_end - uri_start;
  strncpy(request->uri = Malloc(uri_size), uri_start, uri_size);

  const char *host_start = strstr(buffer, "Host: ");
  if (!host_start) {
    app_error("Parse host error\n");
    return 0;
  }

  host_start += 6;
  const char *host_end = strchr(host_start, ':');
  if (!host_end) {
    app_error("Parse host error\n");
    return 0;
  }
  size_t host_size = host_end - host_start;
  strncpy(request->host = Malloc(host_size), host_start, host_size);

  const char *port_start = host_end + 1;
  const char *port_end = strchr(port_start, '\n');
  if (!port_end) {
    app_error("Parse port error\n");
    return 0;
  }
  size_t port_size = port_end - port_start;
  strncpy(request->port = Malloc(port_size), port_start, port_size);
  return 1;
}

void FreeHTTPRequest(HTTPRequest *ptr) {
  free(ptr->uri);
  free(ptr->host);
  free(ptr->port);
}

void DispHTTPRequestStruct(const HTTPRequest *request) {
  fprintf(stdout, "uri=%s\n", request->uri);
  fprintf(stdout, "host=%s\n", request->host);
  fprintf(stdout, "port=%s\n", request->port);
}

char *GetBroswerRequest(int sock_fd, size_t *rec_size) {
  size_t req_buf_size = 1000;
  char *request_buf = Malloc(req_buf_size);
  char *read_buf = Malloc(1000);
  *rec_size = 0;
  while (1) {
    size_t size = 1000;
    if (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, -1, 0) == -1) {
      DebugStr("Broswer closed socket.\n");
      Free(read_buf);
      Free(request_buf);
      return NULL;
    }
    DebugStr("Recv %zu\n", size);
    if (*rec_size + size > req_buf_size) {
      req_buf_size = *rec_size + size + 1000;
      Realloc(request_buf, req_buf_size);
    }
    strncpy(request_buf + *rec_size, read_buf, size);
    *rec_size += size;
    if (strstr(request_buf, "\r\n\r\n")) { // end of request
      Free(read_buf);
      return request_buf;
    }
    DebugStr("Current Receive:%s", request_buf);
    DebugStr("Cannot find end\n");
  }
}