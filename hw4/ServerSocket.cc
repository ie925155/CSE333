/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);


ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

void PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  std::cout << "Socket [" << fd << "] is bound to:" << std::endl;
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    std::cout << " IPv4 address " << astring;
    std::cout << " and port " << htons(in4->sin_port) << std::endl;

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV4 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
    inet_ntop(AF_INET, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    std::cout << " IPv6 address " << astring;
    std::cout << " and port " << htons(in6->sin6_port) << std::endl;

  } else {
    std::cout << " ???? address and port ????" << std::endl;
  }
}

bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd".

  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = ai_family;      // allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "INADDR_ANY"
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  struct addrinfo *result;
  char buffer [32];
  sprintf(buffer, "%d", port_);
  int res = getaddrinfo(NULL, buffer, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
    std::cerr << "getaddrinfo() failed: ";
    std::cerr << gai_strerror(res) << std::endl;
    return false;
  }


  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int fd = -1;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
      fd = 0;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(fd, rp->ai_addr, rp->ai_addrlen);

      sock_family_ = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(fd);
    fd = -1;
  }
  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // Did we succeed in binding to any addresses?
  if (fd == -1) {
    // No.  Quit with failure.
    std::cerr << "Couldn't bind to any addresses." << std::endl;
    return false;
  }

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << strerror(errno) << std::endl;
    close(fd);
    return false;
  }
  listen_sock_fd_ = *listen_fd = fd;
  return true;
}

bool ServerSocket::Accept(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dnsname,
                          std::string *server_addr,
                          std::string *server_dnsname) {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  int client_fd = accept(listen_sock_fd_,
                         reinterpret_cast<struct sockaddr *>(&caddr),
                         &caddr_len);
  if (client_fd < 0) {
    std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
    return false;
  }
  struct sockaddr *addr = reinterpret_cast<struct sockaddr *>(&caddr);
  char astring[INET_ADDRSTRLEN];
  struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
  inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
  std::cout << " IPv4 address " << astring;
  std::cout << " and port " << htons(in4->sin_port) << std::endl;
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, caddr_len, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
    return false;
  }
  std::cout << " DNS name: " << hostname << std::endl;
  *client_addr = astring;
  *accepted_fd = client_fd;
  *client_port = htons(in4->sin_port);
  *client_dnsname = hostname;

  char hname[1024];
  hname[0] = '\0';
  struct sockaddr_in srvr;
  socklen_t srvrlen = sizeof(srvr);
  char addrbuf[INET_ADDRSTRLEN];
  getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
  inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
  std::cout << addrbuf;
  // Get the server's dns name, or return it's IP address as
  // a substitute if the dns lookup fails.
  getnameinfo((const struct sockaddr *) &srvr,
              srvrlen, hname, 1024, NULL, 0, 0);
  std::cout << " [" << hname << "]" << std::endl;
  *server_addr = addrbuf;
  *server_dnsname = hname;
  return true;
}

}  // namespace hw4
