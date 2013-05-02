/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "proxy-constants.h"
                            

using namespace std;
void server_greeting (const struct addrinfo *p); 
void *get_in_addr(struct sockaddr *sa);
in_port_t get_in_port(struct sockaddr *sa);
int ConnectToRemoteHost(const string host , const int port); 
int SendMsg(int sockfd, char *buf, int *len);

int main (int argc, char *argv[])
{
  // command line parsing
  //
  
   struct sockaddr_storage their_addr;
   socklen_t addr_size;

   struct addrinfo hints 
       , *res //result returned from calling addressinfo
       , *p; 

   int listen_socketfd , ret_value;
   char ipstr[INET6_ADDRSTRLEN];
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
    if ((ret_value = getaddrinfo("localhost", PORT_NUMBER, &hints, &res)) != 0) {
              cerr << "getaddrinfo: " <<  gai_strerror(ret_value) << endl;
              exit(1);
    }
   //getaddrinfo("lnxsrv03.seas.ucla.edu",PORT_NUMBER, &hints, &res);

   //make a socket,bind it, and listen on it:
   
    //note that ai means address info
    //iterate through all possible results and bind to first one that is available
    for(p = res; p != NULL; p = p->ai_next) {

        if((listen_socketfd = socket(res->ai_family,res->ai_socktype,
          res->ai_protocol))==  SOCKET_ERROR) {
              cerr << "ERROR opening socket" << endl;
              continue;
        }

        if (setsockopt(listen_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes,
              sizeof(int)) == -1) {
               perror("setsockopt");
                    exit(1);
        }


        
        if( bind(listen_socketfd, res->ai_addr,res->ai_addrlen) == SOCKET_ERROR) {
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

    if(listen(listen_socketfd, BACKLOG) == SOCKET_ERROR) {
        cerr << "ERROR listening" << endl;
        exit(1);
    }
   

    server_greeting(res);
    freeaddrinfo(res);


    while(1) { //main accept loop
         int new_fd;
         addr_size = sizeof their_addr;
         new_fd = accept(listen_socketfd, (struct sockaddr *)&their_addr, &addr_size);
         if(new_fd == SOCKET_ERROR) {
               cerr << "ERROR on accept" << endl;
               //trie creating a new socket
               continue;
         }

        inet_ntop(their_addr.ss_family,
        get_in_addr((struct sockaddr *)&their_addr),
        ipstr, sizeof ipstr);

        cout << "Accept: got a Connection from " << ipstr << endl;


         HttpRequest req;
         while(1) {

               int numbytes;
               string msg; 
               int total_bytes = 0;
               char buf[MAX_DATA_SIZE]; 
               char out[MAX_DATA_SIZE]; 

               //do {
                 if((numbytes = recv(new_fd,buf,MAX_DATA_SIZE - 1,0)) == -1) {
                   perror("recv");
                 }

                 msg += buf;
                 total_bytes += numbytes;
               //} while (numbytes != 0);

               msg += '\0';

               //if the connection was closed
               if(total_bytes == 0) {
                  close(new_fd);
                  break;
               }


               //TODO: only close after timeout
               //or header tellss us to close connection
               

              req.ParseRequest(&(msg[0]),numbytes);
              req.FormatRequest(out); 
              cout << out << endl;

               try {
                 int cxn_fd = ConnectToRemoteHost(req.GetHost(),req.GetPort());
                 //then listen 
                 //cout << cxn_fd;

               } catch (int e) {

               }
               //send to remote host
               //rcv response
               //TODO: cache
               //send back to client
         }

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
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr)
    , ipstr, sizeof ipstr);
    cout <<
    "*****************************************************************************" << endl <<
    "*****************************************************************************" << endl <<
    "* uber HTTP PROXY                         *                                  " << endl <<
    "* ------------------------------------------------------------------------- *" << endl <<
    "*****************************************************************************" << endl;
    cout  << ipver << " : " <<  ipstr << ":" ;
    //convert network byte order to host byte horder:
    printf("%d\n",ntohs(get_in_port((struct sockaddr *)p->ai_addr)));
    cout <<
    "*****************************************************************************" << endl;
    

}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
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

int SendMsg(int sockfd, char *buf, int *len)
{
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

/* ConnectToRemote
 *
 * @param string host - remote hostname
 * 
 * @return int - socketfd corresponding to connection socket 
 * throws CONNECTION_ERR and INVALID_HOST
 */
int ConnectToRemoteHost(const string host , const int  port) {

	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

  stringstream  PORT ;
  PORT << port;

	if ((rv = getaddrinfo(&(host[0]), &((PORT.str())[0]), &hints, &servinfo)) != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		//return 1;
    throw INVALID_HOST;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			//perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			//perror("client: connect");
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

	freeaddrinfo(servinfo); // all done with this structure
  return sockfd;
}





