/*
 * proxy-constants
 *
 * this file contains libraries
 * and predefined constants for the proxy limitations
 */
#ifndef _PROXY_CONSTANTS_H_
#define _PROXY_CONSTANTS_H_

#include "http-request.h"
#include "http-response.h"

// sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

// For threading and cache
#include <unordered_map>
#include <mutex>
#include <thread>

// For date and time
#include <ctime>
#include <time.h>

/** ==== **/
//CUSTOM EXCEPTIONS
enum proxy_exception_t {
  CONNECTION_ERR,
  INVALID_HOST,
  INTERNAL_SERVER_ERR // ie server may terminate prematurely

};

/** ==== **/

#define END_OF_HEADERS "\r\n\r\n"
#define END_OF_HEADERS_LEN 4

// cache specific headers
#define CONTENT_LENGTH "Content-Length"
#define IF_MODIFIED_SINCE "If-Modified-Since"
#define EXPIRES "Expires"
#define LAST_MODIFIED "Last-Modified"
#define NOT_MODIFIED "Not Modified"


// HTTP response codes
#define INVALID_METHOD "Request is not GET"

#define NOT_IMPLEMENTED_CODE "501"
#define NOT_IMPLEMENTED "Not Implemented"

#define BAD_REQUEST_CODE "400"
#define BAD_REQUEST "Bad Request"

#define INTERNAL_SERVER_ERROR_CODE "500"
#define INTERNAL_SERVER_ERROR "Internal Server Error"


#define SOCKET_ERROR -1
#define MAX_DATA_SIZE 4096
#define PORT_NUMBER "16290" // the listening port
                            // for client connections
                            // TODO: this number should be reset to 14805

#define REMOTE_CXN_LMT 100 // the number of simultaneous connections
                           // allowed to remote servers

#define CLIENT_CXN_LMT 10 //the incoming connection limit

#define BACKLOG 2 * CLIENT_CXN_LMT        
                          //the  number of connections 
                          //that can be queued on listening  socket 
                          //we make the backlog a little larger
                          //to accommadte for more connections

#define THREAD_LMT CLIENT_CXN_LMT //the number of threads our
                                   //proxy runs

#define TIMEOUT 15000          //in millseconds  
                               //the default connection time out of 
                               //Apache 2.0 httpd
                               //timeout will be broken into smaller intervals
                               //to check if connection is still active
                                   

#endif 
