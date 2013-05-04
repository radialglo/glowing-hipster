/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "proxy-constants.h"

using namespace std;

typedef struct Metadata {
  time_t last_modified;
  time_t expires;
  string str_last_modified;
  string response;
} meta_t;

typedef pair<string, meta_t> cache_record_t;
typedef unordered_map<string, meta_t> cache_t;
  
typedef pair<string, meta_t> cache_record_t;
typedef unordered_map<string, meta_t> cache_t;
void server_greeting (const struct addrinfo *p); 
void *get_in_addr(struct sockaddr *sa);
in_port_t get_in_port(struct sockaddr *sa);
int ConnectToRemoteHost(const string host , const int port); 
int CreateProxyListener(const char* port);
int SendMsg(int sockfd, char *buf, int *len);
string GetMsgBody(int sockfd, size_t total, size_t len); 
void ClientHandler(int client_fd);
string BuildErrorMsg(const string &code, const string &msg); 
bool IsNewerTime(time_t time_a, time_t time_b);
time_t StrToTime(string &date_string); 
void AddToCache(cache_t &cache, const string key, const string response); 

cache_t cache;
mutex m;

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

  boost::thread t;
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
    cout << "Accept: got a Connection from " << ipstr << " on port" << new_fd << endl;

    // TODO: be careful if new_fd is copied and not referenced?
    t = boost::thread(ClientHandler, new_fd);
    //ClientHandler(new_fd);
    t.detach();
  }

  //make sure we close proxy port
  close(listen_socketfd);

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


// compare time
bool IsNewerTime(time_t time_a, time_t time_b) {
  return difftime(time_a, time_b) > 0.0 ? true : false;
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

int SendMsg(int sockfd, char *buf, size_t *len) {

  size_t total = 0;        // how many bytes we've sent
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

/* GetMsgBody 
 *
 * gets the full or remaining  message unless there is an error encountered
 *
 * @param sockfd - fd to where message body is hosted 
 * @param expected length of message
 */

string GetMsgBody(int sockfd, size_t total, size_t len) {

  string msg_body;
  int n;
  char buf[MAX_DATA_SIZE];

  while(total < len) {
    n = recv(sockfd, buf, MAX_DATA_SIZE - 1, 0);
    if (n == -1) {  
     throw INTERNAL_SERVER_ERR;
    } else if (n == 0) {
      cerr << "*** WARNING: BAD HANDLING IN GetMsgBody\n";
      break; 
    }

    total += n;
    msg_body += buf;
  }

  return msg_body;
} 

/* StrToTime
 *
 * @param date_string 
 *
 * @return time_t coverted time object
 */
time_t StrToTime(string &date_string) {
  struct tm tm;

   //Sat, 04 May 2013 14:55:17 GMT

  //for now assume everything is GMT
  if (strptime(date_string.c_str(),"%a, %d %b %Y %H:%M:%S GMT", &tm) == NULL) {

    //throw(DATE_PARSE_ERR);
    cerr << "*** Unable to convert " << date_string << endl ;

  }

  cout << endl << "SUCCESSFULLY PARSED " << date_string << endl;

  return  mktime(&tm);
}

/* AddToCache
 *
 *
 */

void AddToCache(cache_t &cache, const string key,const string response) {
  
  HttpResponse resp;
  resp.ParseResponse(&(response[0]),response.length());
  
  //build metadata object
  meta_t meta_data;
  string str_last_modified = resp.FindHeader(LAST_MODIFIED),
         str_expires = resp.FindHeader(EXPIRES);

  meta_data.str_last_modified = resp.FindHeader(LAST_MODIFIED);
  meta_data.last_modified = StrToTime(str_last_modified);
  meta_data.expires = StrToTime(str_expires);
  meta_data.response = response;

  cache_record_t record = {key, meta_data};
  cache.insert (record);

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
    
    if (::bind(listen_socketfd, p->ai_addr, p->ai_addrlen) == SOCKET_ERROR) {
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

  //printf("proxy socket %d: connecting to %s\n\n", sockfd, s);

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
 *
 * parses client request
 * contacts remmote host
 * and sends responses back to client
 *
 * @param int client_fd - socket fd of client
 *
 */
void ClientHandler(int client_fd) {

  for (;;) {

    HttpRequest req;
    int num_bytes = 0;
    char buf[MAX_DATA_SIZE];
    string req_msg;
    size_t end_of_req_headers = string::npos; 
 
    // Get Client Request
    // TODO: we assume one HTTP GET per recv
    // WE MAY NEED TO MEMMEM IF THERE ARE MULTIPLE REQUESTS PER RECV
    memset(buf, 0, sizeof(buf));
    //make sure that we have retrieved full request before continuing
    while  ((num_bytes = recv(client_fd, buf, MAX_DATA_SIZE-1, 0)) > 0 ) {

       req_msg.append(buf, num_bytes);

       //if we found the end of the headers 
       if((end_of_req_headers = req_msg.find(END_OF_HEADERS)) != string::npos) {
         break;
       }

    }
    
    if (num_bytes == SOCKET_ERROR) {
      perror("client recv");
      break;
    }

    //if the connection was closed
    if (num_bytes == 0) {
      cout << "Closing Connection" << endl;
      break;
    }
 
    // Parse Client Request
    try { 
      req.ParseRequest(&(req_msg[0]), req_msg.length());
    } catch (ParseException ex) {
      cerr << "Exception: " << ex.what() << endl; 

      // TODO: send proper error response
      string err_msg;
      if(strcmp(ex.what(), INVALID_METHOD) == 0) {
        err_msg = BuildErrorMsg(NOT_IMPLEMENTED_CODE, NOT_IMPLEMENTED);
      } else {
        err_msg = BuildErrorMsg(BAD_REQUEST_CODE, BAD_REQUEST);
      }

      size_t err_msg_len = err_msg.length();
      SendMsg(client_fd, &(err_msg[0]), &err_msg_len);
      break;
    }

    // See if response already cached
    string key = req.GetHost() + req.GetPath();
    cache_t::iterator it;
    
    m.lock();
    //If we have found the key in the cache
    //determine wheter or not we can return the cached value
    if ( (it = cache.find(key)) != cache.end()) {

      string modified_since = req.FindHeader(IF_MODIFIED_SINCE);
      if (modified_since == "") {
        // Check if current time is newer than expires time
        time_t t = time(NULL);
        if (IsNewerTime(mktime(gmtime(&t)), it->second.expires)) {
          // Send request to host with added If-Modified-Since with Last-Modified
          req.AddHeader(IF_MODIFIED_SINCE, it->second.str_last_modified);
        } else {
        //if the cache is not stale  then send 
        //the cached value back to client

        string response = it->second.response ;
        //resp.AddH
        size_t response_len = response.length();
        SendMsg(client_fd, &(response[0]), &response_len);
        cout << endl <<  "Message sent from CACHE " << endl;
        continue;

        }

        m.unlock();
      } else { // Always send request to host
        m.unlock();
        // Convert the string to time_t 
        struct tm tm;
        if (strptime(modified_since.c_str(), "%a, %d %b %Y %H:%M:%S", &tm) == NULL)
          cerr << "*** Bad If-Modified-Since date\n";
        time_t t = mktime(&tm);
        cerr << t << endl;

        // Always send request to host
      }
      
    } 
    // TODO: return proper HTTP response with headers
    //proceed normally with contacting remote host
    m.unlock();

    char formatted[MAX_DATA_SIZE]; 
    memset(formatted, 0, sizeof(formatted));
    req.FormatRequest(formatted); 
    cout << "*** Formatted Request: " << formatted;
    cout << "/** end  Client Request **/" << endl;


    // Connect to remote host 
    int remote_fd;
    try {
      remote_fd = ConnectToRemoteHost(req.GetHost(),req.GetPort());
      //then listen 
      //cout << remote_fd;
    } catch (int e) {
      cout << "*** ERROR Connecting to Remote Host" << endl;

      string err_msg;
      if (e == CONNECTION_ERR) {
        err_msg = BuildErrorMsg(INTERNAL_SERVER_ERROR_CODE, INTERNAL_SERVER_ERROR);
      } else if (e ==  INVALID_HOST) {
        err_msg = BuildErrorMsg(BAD_REQUEST_CODE, BAD_REQUEST);
      }

      size_t err_msg_len = err_msg.length();
      SendMsg(client_fd, &(err_msg[0]), &err_msg_len);
      close(client_fd);
      return;
    }

    cout << "*** Successfully connected to remote host" << endl;
    //send to remote host
    size_t req_msg_len = req.GetTotalLength();
    SendMsg(remote_fd, formatted, &req_msg_len);
    cout <<  "*** SENT REQUEST TO HOST:" << endl << formatted;
    
    // Receive host response
    //clear buffers before using them again
    memset(buf, 0, sizeof buf);
    memset(formatted, 0, sizeof(formatted));

    string response;

    // TODO:
    // Assume one reseponse per recv
    int resp_bytes = 0;
    size_t end_of_headers = string::npos; 
    while  ((resp_bytes = recv(remote_fd, buf, MAX_DATA_SIZE-1, 0)) > 0 ) {

       response.append(buf, resp_bytes);

       //if we found the end of the headers 
       if((end_of_headers = response.find(END_OF_HEADERS)) != string::npos) {
         break;
       }

    }

    //keep track of its position
    end_of_headers += END_OF_HEADERS_LEN;

    
    if (resp_bytes == SOCKET_ERROR) {
      fprintf(stderr, "recv: numbytes = %d\n", num_bytes);
      //TODO: SEND ERR BACK TO HOST
      exit(1);
    }
    cout << "*** RECEIVED RESPONSE FROM HOST\n";
    //printf("proxy socket %d: received \n'%s'\n",remote_fd, &(resp_msg[0]));

    HttpResponse resp;
    try {
      resp.ParseResponse(&(response[0]),response.length());
    } catch (ParseException e) {

    }

    cout << "*** CHECKING FOR CONTENT" << endl;
    //after we get response check for message body
    string content_length_str = resp.FindHeader(CONTENT_LENGTH);
    int content_length = 0;
    if (content_length_str != "")
      content_length = stoi(content_length_str);
    int remaining_content_length = 0;

    //check to see if content was already retrieved from buffer
    //if it was already retrieved then it will be a substring of 
    //the response
    string msg_body = response.substr(end_of_headers, content_length);
    remaining_content_length = content_length - msg_body.length();

    
    //otherwise get the rest of the message body
    msg_body = "";
    if(remaining_content_length != 0) {

      try {
        msg_body += GetMsgBody(remote_fd, msg_body.length(), content_length);
        //add the message body we recieved to the response
        response += msg_body;
      } catch (int e) {
        //TODO: handle retrieval of body failing
        cout << "Error" << endl;
      }
      
    }

    //by this time response will have been fully retrieved

    //TODO: add all content to cache
    m.lock();
    if ( (it = cache.find(key)) != cache.end()) {
      // Update the cache value
      string last_modified_str = resp.FindHeader(LAST_MODIFIED);
      if (last_modified_str != "") {
        it->second.last_modified = StrToTime(last_modified_str); 
      }

      string expires_str = resp.FindHeader(EXPIRES);
      if (expires_str != "") {
        //if (strptime(expires_str.c_str(),"%a %d %b %Y %H:%M:%S", &tm) == NULL)
        //  cerr << "*** Bad Expires date\n";
        it->second.expires = StrToTime(expires_str); 
      }
      
      it->second.response = response;
      
      //TODO: update cache
      
    } else { 
      //add file to cache
      AddToCache(cache, key, response); 
    }

    m.unlock();

    cout << "*** RESPONSE MESSAGE SENT TO CLIENT" << endl;
    
    cout << response << endl;;

    size_t response_len = response.length();
    SendMsg(client_fd, &(response[0]), &response_len);
    cout << "*** RESPONSE LENGTH " << response_len << endl;

    //for simplicity close remote each time
    close(remote_fd);
    //break;
  }
  
  //if there are any errors
  //break out of the for loop and
  //terminate connection

  //if client has closed connection
  //then we can also close their fd
  close(client_fd);
}

