/*
 * proxy-constants
 *
 * this file contains libraries
 * and predefined constants for the proxy limitations
 */
#ifndef _PROXY_CONSTANTS_H_
#define _PROXY_CONSTANTS_H_

#include <iostream>
#include "http-request.h"
#include "http-response.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
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
                                   

#endif 
