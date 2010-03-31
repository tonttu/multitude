/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "TCPServerSocket.hpp"
#include "TCPSocket.hpp"

#include <Radiant/Trace.hpp>

#include <string.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#include <WinPort.h>
#endif

namespace Radiant {

  TCPServerSocket::TCPServerSocket()
    : m_fd(-1)
  {}

  TCPServerSocket::~TCPServerSocket()
  {}

  int TCPServerSocket::open(const char * host, int port, int maxconnections)
  {
    close();

    m_host = host ? host : "";
    m_port = port;

    m_fd = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if(m_fd < 0){
      return errno;
    }

    struct sockaddr_in server_address;

    bzero( & server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((unsigned short)(port));

    if(!host || strlen(host) == 0)
      server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    else {
      in_addr * addr = TCPSocket::atoaddr(host);
      
      if(!addr)
	return EHOSTUNREACH;
      
      server_address.sin_addr.s_addr = addr->s_addr;
      // server_address.sin_addr.s_addr = inet_addr(host);
    }

    //puts("Binding");

    if(bind(m_fd, ( struct sockaddr * ) & server_address,
	    sizeof ( server_address ) ) < 0 ){
      close();
      return errno;
    }

    //puts("Listening");
    
    if(::listen(m_fd, maxconnections) != 0) {
      close();
      return errno;
    }

    return 0;
  }

  bool TCPServerSocket::close()
  {
    if(m_fd < 0)
      return false;

#ifndef WIN32
    ::close(m_fd);
#else
    ::closesocket((SOCKET)m_fd);
#endif

    m_fd = -1;

    return true;
  }
   
  bool TCPServerSocket::isPendingConnection(unsigned waitMicroSeconds)
  {
    if(m_fd < 0)
      return false;

#ifndef WIN32
    struct pollfd pfd;
    bzero( & pfd, sizeof(pfd));
    pfd.fd = m_fd;
    pfd.events = POLLIN;
    poll(&pfd, 1, waitMicroSeconds / 1000);
    return pfd.revents & POLLIN;
#else
    // -- emulate using select()
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = waitMicroSeconds;
    fd_set readfds;
    bzero( & readfs, sizeof(readfs));
    FD_ZERO(&readfds);
#pragma warning (disable:4127 4389)  
    FD_SET(m_fd, &readfds);
#pragma warning (default:4127 4389)
    int status = select(m_fd, &readfds, 0,0, &timeout);
    if (status < 0)
      return false;
//    char data;
	int isset = (FD_ISSET(m_fd, &readfds))/* && (recv(m_fd, &data, 1, MSG_PEEK) <= 0))*/;
	return isset != 0;
#endif
  }

  TCPSocket * TCPServerSocket::accept()
  {
    sockaddr newAddress;
    socklen_t addressLength(sizeof(newAddress));

    int fd = ::accept(m_fd, (sockaddr *) & newAddress, & addressLength);

    if(fd < 0)
      return 0;

    TCPSocket * sock = new TCPSocket(fd);
    
    return sock;
  }

} 
