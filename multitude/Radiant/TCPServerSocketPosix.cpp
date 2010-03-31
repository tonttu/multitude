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

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>

namespace Radiant
{

  class TCPServerSocket::D {
    public:
      D() : m_fd(-1), m_port(0) {}

      int m_fd;
      int m_port;
      std::string m_host;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  TCPServerSocket::TCPServerSocket()
  {
    m_d = new D();
  }
  
  TCPServerSocket::~TCPServerSocket()
  {
    delete m_d;
  }

  int TCPServerSocket::open(const char * host, int port, int maxconnections)
  {
    close();

    m_d->m_host = host ? host : "";
    m_d->m_port = port;

    m_d->m_fd = socket ( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    if(m_d->m_fd < 0){
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
    }

    if(bind(m_d->m_fd, ( struct sockaddr * ) & server_address,
	    sizeof ( server_address ) ) < 0 ){
      close();
      return errno;
    }

    if(::listen(m_d->m_fd, maxconnections) != 0) {
      close();
      return errno;
    }

    return 0;
  }

 bool TCPServerSocket::close()
  {
    if(m_d->m_fd < 0)
      return false;

    ::close(m_d->m_fd);

    m_d->m_fd = -1;

    return true;
  }

  bool TCPServerSocket::isOpen() const
  {
    return (m_d->m_fd > 0);
  }

  bool TCPServerSocket::isPendingConnection(unsigned int waitMicroSeconds)
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    bzero( & pfd, sizeof(pfd));
    pfd.fd = m_d->m_fd;
    pfd.events = POLLIN;
    poll(&pfd, 1, waitMicroSeconds / 1000);

    return pfd.revents & POLLIN;
  }

  TCPSocket * TCPServerSocket::accept()
  {
    sockaddr newAddress;
    socklen_t addressLength(sizeof(newAddress));

    bzero( & newAddress, sizeof(newAddress));

    int fd = ::accept(m_d->m_fd, (sockaddr *) & newAddress, & addressLength);

    if(fd < 0)
      return 0;

    TCPSocket * sock = new TCPSocket(fd);
    
    return sock;
  }

}

