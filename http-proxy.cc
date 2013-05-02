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

  printf("proxy socket %d: connecting to %s\n", sockfd, s);

  freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}

/* ClientHandler
 * @param int client_fd - socket fd of client
 *
 */
void ClientHandler(int client_fd) {

  HttpRequest req;
  for (;;) {

    string msg; 
    int total_bytes = 0;
    int numbytes = 0;
    char buf[MAX_DATA_SIZE]; 
    char out[MAX_DATA_SIZE]; 
 
    //do {
    if((numbytes = recv(client_fd, buf, MAX_DATA_SIZE - 1, 0)) == -1) {
      perror("recv");
    }
 
    msg += buf;
    total_bytes += numbytes;
    //} while (numbytes != 0);
 
    msg += '\0';
 
    //if the connection was closed
    if(total_bytes == 0) {
      close(client_fd);
      break;
    }
 
 
    //TODO: only close after timeout
    //or header tellss us to close connection
    
    try { 
      req.ParseRequest(&(msg[0]),numbytes);
    } catch (ParseException ex) {
      cerr << "Exception: " << ex.what() << endl; 
      // TODO: send proper error response
      

    }
    req.FormatRequest(out); 
    cout << out << endl;

    // TODO: request is not properly setting host and port number
    cout << "Host: " << req.GetHost() << " Port: " << req.GetPort() << endl;
 
    int remote_fd;
    try {
      remote_fd = ConnectToRemoteHost(req.GetHost(),req.GetPort());
      //then listen 
      cout << remote_fd;
    } catch (int e) {
 
    }
    //send to remote host
    int msg_len = req.GetTotalLength();
    SendMsg(remote_fd, out, &msg_len);
    
    //rcv response
    if ((numbytes = recv(remote_fd, buf, MAX_DATA_SIZE-1, 0)) == SOCKET_ERROR || numbytes == 0) {
      fprintf(stderr, "recv: numbytes = %d\n", numbytes);
    }

    buf[numbytes] = '\0';
    printf("proxy socket %d: received '%s'\n", client_fd, buf);

    //TODO: cache
    //send back to client
    close(client_fd);
  }
}



