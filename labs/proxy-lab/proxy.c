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
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
typedef struct {
  char *path;
  char *host;
  char *port;
}HTTPRequest;

typedef struct {
  char *status;
  char *date;
  char *size;
}HTTPResponse;

int HTTPRequestParser(const char *buffer, size_t size, HTTPRequest *request);
void FreeHTTPRequest(HTTPRequest *ptr);
void FreeHTTPREsponse(HTTPResponse *ptr);
void DispHTTPRequestStruct(const HTTPRequest *request);
char *GetBroswerRequest(int sock_fd, size_t *rec_size);
int ForwardBroswerRequest(int sock_fd, const char *request, size_t size);
char *GetHostResponse(int sock_fd, size_t *rec_size, HTTPResponse *response);
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
    DebugStr("GetBroswerRequest\n");
    char *request_buf = GetBroswerRequest(broswer_fd, &size);
    assert(request_buf);
    DebugStr("Receive Request:\n");
    DebugStr("%s\n", request_buf);

    HTTPRequest request;
    DebugStr("Parse HTTP\n");
    if (HTTPRequestParser(request_buf, size, &request)) {
      DebugStr("\nResult of HTTPRequestParser:\n");
      DispHTTPRequestStruct(&request);
      int fd = ConnectTo(request.host, request.port, -1, 0);
      assert(ForwardBroswerRequest(fd, request_buf, size) == 1);

      HTTPResponse response;
      size = 0;
      char *response_buf = GetHostResponse(fd, &size, &response);
      assert(response_buf);
      DebugStr("%s\n", response_buf);
    }
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

  request->path = NULL;
  request->host = NULL;
  request->port = NULL;

  if (!strstr(buffer, "GET")) {
    app_error("Currently only support GET Method\n");
    return 0;
  }

  const char *path_start = buffer + 4;
  const char *path_end = strchr(path_start, ' ');
  if (!path_end) {
    app_error("Parse path error\n");
    return 0;
  }

  size_t path_size= path_end - path_start;
  strncpy(request->path = Malloc(path_size+1), path_start, path_size);
  request->path[path_size] = '\0';
  const char *host_start = strstr(buffer, "Host: ");
  if (!host_start) {
    app_error("Parse host error\n");
    return 0;
  }

  host_start += 6;
  const char *host_end = strpbrk(host_start, ":\r\n");
  if (!host_end) {
    app_error("Parse host error\n");
    return 0;
  }

  size_t host_size = host_end - host_start;
  strncpy(request->host = Malloc(host_size+1), host_start, host_size);
  request->host[host_size] = '\0';

  if (*host_end != ':') {
    strncpy(request->port = Malloc(3), "80", 3);
    return 1;
  }

  const char *port_start = host_end + 1;
  const char *port_end = strchr(port_start, '\n');
  if (!port_end) {
    app_error("Parse port error\n");
    return 0;
  }

  size_t port_size = port_end - port_start;
  strncpy(request->port = Malloc(port_size+1), port_start, port_size);
  request->port[port_size] = '\0';
  return 1;
}

void FreeHTTPRequest(HTTPRequest *ptr) {
  if (ptr->path) free(ptr->path);
  if (ptr->host) free(ptr->host);
  if (ptr->port) free(ptr->port);
}

void DispHTTPRequestStruct(const HTTPRequest *request) {
  fprintf(stdout, "path=%s\n", request->path);
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
    if (*rec_size + size >= req_buf_size) {
      req_buf_size = *rec_size + size + 1000;
      request_buf = Realloc(request_buf, req_buf_size);
    }
    strncpy(request_buf + *rec_size, read_buf, size);
    *rec_size += size;
    if (strstr(request_buf, "\r\n\r\n")) { // end of request
      Free(read_buf);
      return request_buf;
    }
    request_buf[*rec_size] = '\0';
  }
}

int ForwardBroswerRequest(int sock_fd, const char *request, size_t size) {
  return SocketSend(sock_fd, request, &size, -1, 0);
}

char *GetHostResponse(int sock_fd, size_t *rec_size, HTTPResponse *response) {
  response->status = NULL;
  response->date = NULL;
  response->size = NULL;

  size_t response_buf_size = 5000;
  char *response_buf = Malloc(response_buf_size);
  char *read_buf = Malloc(1000);
  *rec_size = 0;
  typedef enum {
    WAIT_FOR_HEADER = 0,
    PROCESS_HEADER = 1,
    WAIT_FOR_CONTENT = 2
  }State;

  State state = WAIT_FOR_HEADER;
  const char *pContent = NULL;
  const char *p = NULL;
  const char *size_start = NULL;
  const char *size_end = NULL;
  size_t header_size = 0;
  size_t content_size = 0;
  size_t size_len = 0;
  int finish = 0;
  while (!finish) {
    size_t size = 1000;
    switch(state) {
      case WAIT_FOR_HEADER:
        switch (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 30000, 0)) {
          case 1: break;
          case 0:
            DebugStr("Timeout\n");
            Free(read_buf);
            Free(response_buf);
            return NULL;
          case -1:
            DebugStr("Host closed socket\n");
            Free(read_buf);
            Free(response_buf);
            return NULL;
        }

        if (*rec_size + size >= response_buf_size) {
          response_buf_size = *rec_size + size + 5000;
          response_buf = Realloc(response_buf, response_buf_size);
        }

        strncpy(response_buf_size + *rec_size, read_buf, size);
        *rec_size += size;
        if ((p = strstr(response_buf, "\r\n\r\n"))) {
          header_size = p - response_buf + 2;
          state = PROCESS_HEADER;
        }
        break;

      case PROCESS_HEADER:
        pContent = p + 4;
        size_start = strstr(response_buf, "Content-Length: ");
        if (size_start) {
          size_start += 16;
          size_end = strpbrk(size_start, "\r\n\0");
          size_len = size_end - size_start;
          strncpy(response->size = Malloc(size_len + 1),
                  size_start, size_len);
          response->size[size_len] = '\0';
          content_size = strtoul(response->size, NULL, 10);
        }

        if (header_size + 2 + content_size > *rec_size) {
          state = WAIT_FOR_HEADER;
        } else {
          finish = 1;
        }
        break;

      case WAIT_FOR_CONTENT:
        switch (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 30000, 0)) {
          case 1: break;
          case 0:
            DebugStr("Timeout\n");
            Free(read_buf);
            Free(response_buf);
            return NULL;
          case -1:
            DebugStr("Host closed socket\n");
            Free(read_buf);
            Free(response_buf);
            return NULL;
        }
        if (*rec_size + size >= response_buf_size) {
          response_buf_size = *rec_size + size + 5000;
          response_buf = Realloc(response_buf, response_buf_size);
        }
        strncpy(response_buf_size + *rec_size, read_buf, size);
        *rec_size += size;
        if (header_size + 2 + content_size == *rec_size) {
          finish = 1;
        }
        break;
    }
  }

  // Get status
  const char *status_end = strpbrk(response_buf, "\r\n\0");
  size_t status_line_len = status_end - (const char*)response_buf;
  strncpy(response->status = Malloc(status_line_len + 1),
          response_buf, status_line_len);
  response->status[status_line_len] = '\0';

  // Get Date
  const char *date_start = strstr(response_buf, "Date: ");
  if (date_start) {
    date_start += 6;
    const char *date_end = strpbrk(date_start, "\r\n\0");
    size_t date_size = date_end - date_start;
    strncpy(response->date = Malloc(date_size + 1), date_start, date_size);
    response->date[date_size] = '\0';
  }

  Free(read_buf);
  return response_buf;
}
