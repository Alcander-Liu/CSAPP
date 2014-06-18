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
  char *connection;
}HTTPRequest;

typedef struct {
  char *status;
  char *date;
  char *size;
}HTTPResponse;

void InitHTTPRequest(HTTPRequest *ptr);
void FreeHTTPRequest(HTTPRequest *ptr);
int HTTPRequestParser(const char *buffer, HTTPRequest *request);
void DispHTTPRequestStruct(const HTTPRequest *request);

void InitHTTPResponse(HTTPResponse *ptr);
void FreeHTTPREsponse(HTTPResponse *ptr);

char *GetBroswerRequest(int sock_fd, size_t *rec_size, HTTPRequest *request);
int ForwardBroswerRequest(int sock_fd, const char *request, size_t size);
char *GetHostResponse(int sock_fd, size_t *rec_size, HTTPResponse *response);
int ForwardHostResponse(int sock_fd, const char *response, size_t size);

void ClientError(void);
int IsTransferEnd(const char *ptr_beg, const char *ptr_end);
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
  while (1) {
    DebugStr("Waiting for broswer connection...\n");
    int broswer_fd = Accept(server_fd, -1, 0, client_addr);
    int broswer_close_con = 0;
    while (!broswer_close_con) {

      DebugStr("Waiting for broswer request...\n");
      HTTPRequest request;
      size_t request_size = 0;
      char *request_buf = GetBroswerRequest(broswer_fd, &request_size, &request);
      if (!request_buf) {
        FreeHTTPRequest(&request);
        broswer_close_con = 1;
        continue;
      }

      DebugStr("Received Broswer Request:\n");
      DispHTTPRequestStruct(&request);

      DebugStr("Trying to connect to host...\n");
      int host_fd = ConnectTo(request.host, request.port, -1, 0);
      FreeHTTPRequest(&request);
      if (host_fd < 0) {
        ClientError();
        continue;
      }

      DebugStr("Trying to forward broswer request...\n");
      if (!ForwardBroswerRequest(host_fd, request_buf, request_size)) {
        DebugStr("Forward broswer error...\n");
        Close(host_fd);
        ClientError();
        Free(request_buf);
        continue;
      }
      Free(request_buf);

      HTTPResponse response;
      size_t response_size = 0;
      char *response_buf = GetHostResponse(host_fd, &response_size, &response);
      if (!response_buf) {
        ClientError();
        Close(host_fd);
        FreeHTTPREsponse(&response);
        continue;
      }
      FreeHTTPREsponse(&response);

      if (!ForwardHostResponse(broswer_fd, response_buf, response_size)) {
        DebugStr("Forward Host response error...\n");
        Free(response_buf);
        break;
      }
      Free(response_buf);
      Close(host_fd);
    }
    Close(broswer_fd);
  }
  exit(0);
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

int HTTPRequestParser(const char *buffer, HTTPRequest *request) {

  if (!strstr(buffer, "GET")) {
    app_error("Currently only support GET Method\n");
    DebugStr("%s\n", buffer);
    ClientError();
    return 0;
  }

  const char *path_start = buffer + 4;
  const char *path_end = strchr(path_start, ' ');
  if (!path_end) {
    app_error("Parse path error\n");
    ClientError();
    return 0;
  }

  size_t path_size= path_end - path_start;
  strncpy(request->path = Malloc(path_size+1), path_start, path_size);
  request->path[path_size] = '\0';
  const char *host_start = strstr(buffer, "Host: ");
  if (!host_start) {
    app_error("Parse host error\n");
    ClientError();
    return 0;
  }

  host_start += 6;
  const char *host_end = strpbrk(host_start, ":\r\n");
  if (!host_end) {
    app_error("Parse host error\n");
    ClientError();
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
    ClientError();
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
  if (ptr->connection) free(ptr->connection);
}

void DispHTTPRequestStruct(const HTTPRequest *request) {
  fprintf(stdout, "path=%s\n", request->path);
  fprintf(stdout, "host=%s\n", request->host);
  fprintf(stdout, "port=%s\n", request->port);
}

void InitHTTPRequest(HTTPRequest *ptr) {
  ptr->path = NULL;
  ptr->host = NULL;
  ptr->port = NULL;
  ptr->connection = NULL;
}

char *GetBroswerRequest(int sock_fd, size_t *rec_size, HTTPRequest *request) {
  InitHTTPRequest(request);

  size_t req_buf_size = 1000;
  char *request_buf = Malloc(req_buf_size);
  char *read_buf = Malloc(1000);
  *rec_size = 0;
  int finish = 0;
  while (!finish) {
    size_t size = 1000;
    if (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 3000, 0) == -1) {
      DebugStr("GetBroswerRequest: broswer closed socket.\n");
      Free(read_buf);
      Free(request_buf);
      return NULL;
    }
    if (*rec_size + size >= req_buf_size) {
      req_buf_size = *rec_size + size + 1000;
      request_buf = Realloc(request_buf, req_buf_size);
    }
    strncpy(request_buf + *rec_size, read_buf, size);
    *rec_size += size;
    if (strstr(request_buf, "\r\n\r\n")) { // end of request
      request_buf[*rec_size] = '\0';
      Free(read_buf);
      finish = 1;
    }
  }

  if (!HTTPRequestParser(request_buf, request)) {
    app_error("Parse HTTP Request Error.\n");
    Free(request_buf);
    return NULL;
  }

  return request_buf;
}

int ForwardBroswerRequest(int sock_fd, const char *request, size_t size) {
  if (SocketSend(sock_fd, request, &size, -1, 0) <= 0) {
    return 0;
  }
  return 1;
}

void InitHTTPResponse(HTTPResponse *response) {
  response->status = NULL;
  response->date = NULL;
  response->size = NULL;
}

char *GetHostResponse(int sock_fd, size_t *rec_size, HTTPResponse *response) {
  InitHTTPResponse(response);

  size_t response_buf_size = 5000;
  char *response_buf = Malloc(response_buf_size);
  char *read_buf = Malloc(1000);
  *rec_size = 0;

  typedef enum {
    WAIT_FOR_HEADER = 0,
    PROCESS_HEADER = 1,
    KNOW_CONTENT_LENGTH = 2,
    CHUNKED_TRANS = 3
  }State;

  State state = WAIT_FOR_HEADER;
  const char *header_tail = NULL;
  size_t header_size = 0;
  size_t content_size = 0;
  size_t total_size = 0;
  int finish = 0;
  while (!finish) {
    if (state == WAIT_FOR_HEADER) {
      size_t size = 1000;
      switch (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 30000, 0)) {
        case 0:
          DebugStr("GetHostResponse: Wait for header timeout\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;

        case -1:
          DebugStr("GetHostResponse: Host close socket\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;
      }

      if (*rec_size + size >= response_buf_size) {
        response_buf_size = *rec_size + size + 1000;
        response_buf = Realloc(response_buf, response_buf_size);
      }
      // Copy the newly received data
      strncpy(response_buf + *rec_size, read_buf, size);
      *rec_size += size;
      response_buf[*rec_size] = '\0';
      if ((header_tail = strstr(response_buf, "\r\n\r\n"))) {
        header_tail += 2;
        header_size = header_tail - response_buf;
        state = PROCESS_HEADER;
      }
    } else if (state == PROCESS_HEADER) {
      const char *content_size_b = strstr(response_buf, "Content-Length: ");
      const char *trans_encoding_b = strstr(response_buf, "Transfer-Encoding: ");
      if (!trans_encoding_b && !content_size_b) {
        DebugStr("GetHostResponse: No Content-Length and Transfer-Encoding\n");
        Free(read_buf);
        Free(response_buf);
        ClientError();
        return NULL;
      }
      if (trans_encoding_b) {
        state = CHUNKED_TRANS;
      } else {
        state = KNOW_CONTENT_LENGTH;
        const char *content_size_e = strpbrk(content_size_b, "\r\n\0");
        size_t content_size_sz = content_size_e - content_size_b;
        strncpy(response->size = Malloc(content_size_sz + 1),
                content_size_b, content_size_sz);
        content_size = strtoul(response->size, NULL, 10);
        total_size = header_size + 2 + content_size;
      }
    } else if (state == KNOW_CONTENT_LENGTH) {
      size_t left = total_size - *rec_size;
      if (!left) {
        finish = 1;
        continue;
      }
      size_t size = left > 1000 ? 1000 : left;
      switch (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 3000, 0)) {
        case 0:
          DebugStr("GetHostResponse: Wait for content timeout\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;
        case -1:
          DebugStr("GetHostResponse: Host close socket.\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;
      }
      if (*rec_size + size >= response_buf_size) {
        response_buf_size = *rec_size + size + 1000;
        response_buf = Realloc(response_buf, response_buf_size);
      }
      strncpy(response_buf + *rec_size, read_buf, size);
      *rec_size += size;
    } else if (state == CHUNKED_TRANS) {

      // Need a parser
      if (IsTransferEnd(header_tail + 2, response_buf + *rec_size)) {
        finish = 1;
        continue;
      }
      size_t size = 1000;
      switch (SocketRecv(sock_fd, read_buf, &size, DONT_WAIT_ALL_DATA, 3000, 0)) {
        case 0:
          DebugStr("GetHostResponse: Wait for content timeout\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;
        case -1:
          DebugStr("GetHostResponse: Host close socket.\n");
          Free(read_buf);
          Free(response_buf);
          ClientError();
          return NULL;
      }
      if (*rec_size + size >= response_buf_size) {
        response_buf_size = *rec_size + size + 1000;
        response_buf = Realloc(response_buf, response_buf_size);
      }
      strncpy(response_buf + *rec_size, read_buf, size);
      *rec_size += size;
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

int ForwardHostResponse(int sock_fd, const char *response, size_t size) {
  if (SocketSend(sock_fd, response, &size, -1, 0) <= 0) {
    return 0;
  }
  return 1;
}

void ClientError(void) {
  assert(NULL);
}

int HexToNum(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  return ch - 'A' + 10;
}
// ptr_beg is the begin pointer of content
// ptr_end is the end pointer of buffer
int IsTransferEnd(const char *ptr_beg, const char *ptr_end) {
  while (ptr_beg < ptr_end) {
    const char *line_end = strstr(ptr_beg, "\r\n");
    if (!line_end) {
      break;
    }
    size_t sz_size = line_end - ptr_beg;
    size_t chunk_size = 0;
    for (int i = 0; i < sz_size; ++i) {
      chunk_size = chunk_size * 16 + HexToNum(*(ptr_beg + i));
    }
    if (chunk_size == 0) {
      return 1;
    }
    ptr_beg += chunk_size;
  }
  return 0;
}

void FreeHTTPREsponse(HTTPResponse *ptr) {
  if (!ptr) return;
  if (ptr->status) free(ptr->status);
  if (ptr->date) free(ptr->date);
  if (ptr->size) free(ptr->size);
}