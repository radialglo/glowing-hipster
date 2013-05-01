/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include "http-request.h"
#include "http-response.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include  <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>




#define PORT_NUMBER "16290" // the listening port
                            // for client connections
                            // TODO: this number should be reset to 14805

#define REMOTE_CXN_LMT 100 // the number of simulatenous connections
                           // allowed to remote servers

#define CLIENT_CXN_LMT 10 //the incoming connection limit

#define BACKLOG 2 * CLIENT_CXN_LMT        
                          //the  number of connections 
                          //that can be queued on listening  socket 
                          //we make the backlog a little larger
                          //to accommadte for more connections

#define PROCESS_LMT CLIENT_CXN_LMT //the number of processes our
                                   //proxy runs

#define TIMEOUT 15000          //in millseconds  
                               //the default connection time out of 
                               // Apache 2.0 httpd
                               // timeout will be broken into smaller intervals
                               // to check if connection is still active
                                   
                            



using namespace std;
void print_ip_and_port (struct addrinfo **res); 
in_port_t get_in_port(struct sockaddr *sa);

int main (int argc, char *argv[])
{
  // command line parsing
  //
  
  struct sockaddr_storage their_addr;
  socklen_t addr_size;

  struct addrinfo hints 
       , *res; //result returned from calling addressinfo

   int listen_socketfd;

   // !! don't forget your error checking for these calls!!
   
   // first, load up address structs with getaddrinfo();
   // note that gethostbyname and getservbyname are depreated
   
   //make sure that hint is cleared
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC; //use whichever IPv4 or IPv6
   hints.ai_socktype = SOCK_STREAM; // use TCP sockets
   hints.ai_flags = AI_PASSIVE; // fill in IP 

   //connects to localhost 127.0.0.1 at PORT_NUMBER
   getaddrinfo("localhost",PORT_NUMBER, &hints, &res);
   //getaddrinfo("lnxsrv03.seas.ucla.edu",PORT_NUMBER, &hints, &res);

   //make a socket,bind it, and listen on it:
   
    //note that ai means address info
   listen_socketfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (listen_socketfd < 0)
          cerr << "ERROR opening socket" << endl;

    
    if( bind(listen_socketfd, res->ai_addr,res->ai_addrlen) < 0)
          cerr << "ERROR binding" << endl;

    if(listen(listen_socketfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
   

    print_ip_and_port(&res);
    freeaddrinfo(res);


    while(1) {
         int new_fd;
         addr_size = sizeof their_addr;
         new_fd = accept(listen_socketfd, (struct sockaddr *)&their_addr, &addr_size);
         cout << "Recieved Connection" << endl;
         if (new_fd < 0)
               cerr << "ERROR on accept" << endl;
         string msg = "Beej was here!";
         int len, bytes_sent = 0;
         len = msg.length();
         while(len > bytes_sent) {
           bytes_sent = send(new_fd, &msg, len, 0);
           if(bytes_sent == -1) {
               perror("send");
               exit(0);
           }
           //cout << bytes_sent << endl;
           len -= bytes_sent;
         }

         //TODO: only close after timeout
         //or header tellss us to close connection
         close(new_fd);

    }


  
  return 0;
}

void print_ip_and_port (struct addrinfo **res) {

  char ipstr[INET6_ADDRSTRLEN];

  struct addrinfo *p = *res;

  for( ;p != NULL; p = p->ai_next) {
        void *addr;
        string ipver;

             // get the pointer to the address itself,
             // different fields in IPv4 and IPv6:
   if (p->ai_family == AF_INET) { // IPv4
     struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
     addr = &(ipv4->sin_addr);
     ipver = "IPv4";

    } else { // IPv6
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);
        ipver = "IPv6";
    }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        cout << ipver << " : " <<  ipstr << ":" ;
        printf("%d\n",ntohs(get_in_port((struct sockaddr *)p->ai_addr)));
        //cout <<ntohs(get_in_port((struct sockaddr *)p->ai_addr));
    }

}

in_port_t get_in_port(struct sockaddr *sa) {

        if (sa->sa_family == AF_INET) {
            return (((struct sockaddr_in*)sa)->sin_port);
        }

        return (((struct sockaddr_in6*)sa)->sin6_port);
}





