/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "proxy-constants.h"

using namespace std;

void server_greeting (const struct addrinfo *p); 
void *get_in_addr(struct sockaddr *sa);
in_port_t get_in_port(struct sockaddr *sa);
int ConnectToRemoteHost(const string host , const int port); 
int CreateProxyListener(const char* port);
int SendMsg(int sockfd, char *buf, int *len);
void ClientHandler(int client_fd);
string BuildErrorMsg(const string &code, const string &msg); 

int main (int argc, char *argv[])
{
  // command line parsing
  
  struct sockaddr_storage their_addr;
  socklen_t addr_size;

  char ipstr[INET6_ADDRSTRLEN];

  int listen_socketfd;
  try { 
    listen_socketfd = CreateProxyListener(PORT_NUMBER);
  }
  catch (int i) {
    cerr << "Failed to create listening socket\n";
  }

  while(1) { //main accept loop

    addr_size = sizeof(their_addr);
    int new_fd = accept(listen_socketfd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == SOCKET_ERROR) {
      cerr << "ERROR on accept" << endl;
      //try creating a new socket
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              ipstr, sizeof ipstr);
    cout << "Accept: got a Connection from " << ipstr << endl;

    ClientHandler(new_fd);

  }
  return 0;
}


void server_greeting (const struct addrinfo *p) {

  char ipstr[INET6_ADDRSTRLEN];
  string ipver;

             // get the pointer to the address itself,
             // different fields in IPv4 and IPv6:
  if (p->ai_family == AF_INET) { // IPv4
    ipver = "IPv4";

  } else { // IPv6
    ipver = "IPv6";
  }

  // convert the IP to a string and print it:
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            ipstr, sizeof ipstr);
  cout <<
  "*****************************************************************************" << endl <<
  "*****************************************************************************" << endl <<
  "* uber HTTP PROXY                         *                                  " << endl <<
  "* ------------------------------------------------------------------------- *" << endl <<
  "*****************************************************************************" << endl;
  cout << ipver << " : " <<  ipstr << ":" ;
  //convert network byte order to host byte horder:
  printf("%d\n",ntohs(get_in_port((struct sockaddr *)p->ai_addr)));
  cout <<
  "*****************************************************************************" << endl;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {

  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


//get port number:
in_port_t get_in_port(struct sockaddr *sa) {

  if (sa->sa_family == AF_INET) {
    return (((struct sockaddr_in*)sa)->sin_port);
  }

  return (((struct sockaddr_in6*)sa)->sin6_port);
}



/* SendMsg
 *
 * sends full message unless there is an error encountered
 *
 * @param sockfd - destination socket file descriptor 
 * @param char *buf , HTTP  message
 * @param length of message
 */

int SendMsg(int sockfd, char *buf, int *len) {

  int total = 0;        // how many bytes we've sent
  int bytesleft = *len; // how many we have left to send
  int n;

  while(total < *len) {
    n = send(sockfd, buf+total, bytesleft, 0);
    if (n == -1) { break; }
    total += n;
    bytesleft -= n;
  }

  *len = total; // return number actually sent here

  return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
} 

/* CreateProxyListener
 *
 * @param const char* port - port to listen on
 *
 * @return int sock_fd - corresponding to listening socket
 * throws CONNECTION_ERR on failure
 */
int CreateProxyListener(const char* port) {
  
  //result returned from calling addressinfo
  struct addrinfo hints, *res, *p; 

  int listen_socketfd, ret_value;
  int yes = 1;

  // !! don't forget your error checking for these calls!!
  
  // first, load up address structs with getaddrinfo();
  // note that gethostbyname and getservbyname are depreated
  
  //make sure that hint is cleared
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; //use whichever IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // use TCP sockets
  hints.ai_flags = AI_PASSIVE; // fill in IP 

  //connects to localhost 127.0.0.1 at PORT_NUMBER
  //TODO: change IP of proxy to NULL - cannot assume 
  //if ((ret_value = getaddrinfo(NULL, port, &hints, &res)) != 0) {
  if ((ret_value = getaddrinfo("localhost", port, &hints, &res)) != 0) {
    cerr << "getaddrinfo: " <<  gai_strerror(ret_value) << endl;
    exit(1);
  }
  //getaddrinfo("lnxsrv03.seas.ucla.edu",PORT_NUMBER, &hints, &res);

  //make a socket, bind it, and listen on it:
  
  //note that ai means address info
  //iterate through all possible results and bind to first one that is available
  for(p = res; p != NULL; p = p->ai_next) {

    if((listen_socketfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) ==  SOCKET_ERROR) {
      cerr << "ERROR opening socket" << endl;
      continue;
    }

    if (setsockopt(listen_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }
    
    if (bind(listen_socketfd, p->ai_addr, p->ai_addrlen) == SOCKET_ERROR) {
      close(listen_socketfd);
      cerr << "ERROR binding" << endl;
      continue;
    }

    break;
  }

  if(p == NULL) {
    cerr << "ERROR server failed to bind" << endl;
    exit(1);
  }

  freeaddrinfo(res); // all done with this structure
  if(listen(listen_socketfd, BACKLOG) == SOCKET_ERROR) {
    cerr << "ERROR listening" << endl;
    exit(1);
  }

  server_greeting(res);
  return listen_socketfd;
}

/* ConnectToRemote
 *
 * @param string host - remote hostname
 * 
 * @return int socketfd - corresponding to connection socket 
 * throws CONNECTION_ERR and INVALID_HOST
 */
int ConnectToRemoteHost(const string host, const int port) {

  int sockfd;  
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  stringstream PORT;
  PORT << port;
  if ((rv = getaddrinfo(&(host[0]), &(PORT.str()[0]), &hints, &servinfo)) != 0) {
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    //return 1;
    throw INVALID_HOST;
  }

  // loop through all the results and connect to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
        p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    //fprintf(stderr, "client: failed to connect\n");
    //return 2;
    throw CONNECTION_ERR;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
      s, sizeof s);

  printf("proxy socket %d: connecting to %s\n\n", sockfd, s);

  freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}

/* Build Error Message
 *
 * @string code - status code
 * @string msg - status message
 *
 * @return string Error Response Message
 */

string BuildErrorMsg(const string &code, const string &msg) {

  char out[MAX_DATA_SIZE]; 
  string ret_string;
  HttpResponse resp;

  resp.SetVersion("1.1");
  resp.SetStatusCode(code);
  resp.SetStatusMsg(msg);
  resp.FormatResponse(out);
  ret_string = out;
  
  cout << "Built ERROR string:" << ret_string << endl;
  return ret_string;
}

/* ClientHandler
 * @param int client_fd - socket fd of client
 *
 */
void ClientHandler(int client_fd) {

  HttpRequest req;
  for (;;) {

    string req_msg, resp_msg; 
    int total_bytes = 0 , numbytes = 0;
    char buf[MAX_DATA_SIZE] , out[MAX_DATA_SIZE]; 
 
    //do {
    if((numbytes = recv(client_fd, buf, MAX_DATA_SIZE - 1, 0)) == -1) {
      perror("recv");
    }
 
    req_msg += buf;
    total_bytes += numbytes;
    //} while (numbytes != 0);
 
    req_msg += '\0';
 
    //if the connection was closed
    if(total_bytes == 0) {
      cout << "Closing Connection" << endl;
      close(client_fd);
      break;
    }
 
 
    //TODO: only close after timeout
    //or header tellss us to close connection
    
    try { 
      req.ParseRequest(&(req_msg[0]),req_msg.length());
    } catch (ParseException ex) {
      cerr << "Exception: " << ex.what() << endl; 
      // TODO: send proper wror response
      string err_msg;
      if(strcmp(ex.what(), INVALID_METHOD) == 0) {
        err_msg = BuildErrorMsg(NOT_IMPLEMENTED_CODE, NOT_IMPLEMENTED);
      } else {
        err_msg = BuildErrorMsg(BAD_REQUEST_CODE, BAD_REQUEST);
      }

      int err_msg_len = err_msg.length();
      SendMsg(client_fd, &(err_msg[0]), &err_msg_len);
      close(client_fd);
      return;
    }
    req.FormatRequest(out); 
    cout << out << endl;
    cout << "/** end  Client Request **/" << endl << endl;

    // TODO: request is not properly setting host and port number
    cout << "Host: " << req.GetHost() << " Port: " << req.GetPort() << endl;
 
    int remote_fd;
    try {
      remote_fd = ConnectToRemoteHost(req.GetHost(),req.GetPort());
      //then listen 
      //cout << remote_fd;
    } catch (int e) {
      cout << "ERROR Connecting to Remote Host" << endl;
 
    }

     cout << "Successfully connected to remote host" << endl;
    //send to remote host
    int req_msg_len = req.GetTotalLength();
    SendMsg(remote_fd, out, &req_msg_len);
    cout <<  "SENT REQUEST TO HOST:" << endl
    << out << endl;
    
    //rcv response
    //clear buffers before using them again
    memset(buf, 0, sizeof buf);
    memset(out, 0, sizeof out);

    if ((numbytes = recv(remote_fd, buf, MAX_DATA_SIZE-1, 0)) == SOCKET_ERROR || numbytes == 0) {
      fprintf(stderr, "recv: numbytes = %d\n", numbytes);
    }

    //buf[numbytes] = '\0';
    resp_msg += buf;
    printf("proxy socket %d: received \n'%s'\n",remote_fd, &(resp_msg[0]));

    HttpResponse resp;
    try {
      resp.ParseResponse(&(resp_msg[0]),resp_msg.length());
    } catch (ParseException e) {

    }
    resp.FormatResponse(out);
    cout << "RESPONSE MESSAGE SENT TO CLIENT" << endl 
    << "'" << out << "'" << endl;
    int resp_msg_len = resp.GetTotalLength();
    resp_msg = out;
    //resp_msg_len = resp_msg.length();
    SendMsg(client_fd, &(resp_msg[0]), &resp_msg_len);
    cout << resp_msg_len << endl;

    //TODO: cache
    //send back to client

    close(client_fd);
    close(remote_fd);
    break;
  }
}



