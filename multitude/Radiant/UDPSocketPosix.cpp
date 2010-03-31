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

#include "UDPSocket.hpp"
#include "TCPSocket.hpp"
#include "Trace.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>

namespace Radiant
{

  class UDPSocket::D {
  public:
    D() : m_fd(-1), m_port(0)
    {
      bzero( & m_server, sizeof(m_server));
    }
    
    int m_fd;
    int m_port;
    std::string m_host;

    struct sockaddr_in m_server;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  UDPSocket::UDPSocket()
  {
    m_d = new D();
  }

  UDPSocket::UDPSocket(int fd)
  {
    m_d = new D();
    m_d->m_fd = fd;
  }

  UDPSocket::~UDPSocket()
  {
    delete m_d;
  }

  int UDPSocket::open(const char * host, int port, bool client)
  {
    if(client)
      return openClient(host, port);
    else
      return openServer(host, port);    
  }

  int UDPSocket::openServer(const char * host, int port)
  {
    close();
   
    m_d->m_host = host;
    m_d->m_port = port;

    m_d->m_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(m_d->m_fd < 0)
      return errno;

    bzero( & m_d->m_server, sizeof(m_d->m_server));
    m_d->m_server.sin_family = AF_INET;
    m_d->m_server.sin_port = htons((short)port);
    m_d->m_server.sin_addr.s_addr = INADDR_ANY;
    int err = bind(m_d->m_fd, (struct sockaddr *) & m_d->m_server,
                   sizeof(m_d->m_server));

    if (err != 0) 
      return err;
    
    return 0;
  }

  int UDPSocket::openClient(const char * host, int port)
  {  
    close();
   
    m_d->m_host = host;
    m_d->m_port = port;

    m_d->m_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(m_d->m_fd < 0)
      return errno;


    bzero( & m_d->m_server, sizeof(m_d->m_server));
    m_d->m_server.sin_family = AF_INET;
    m_d->m_server.sin_port = htons((short)port);

    in_addr * addr = TCPSocket::atoaddr(host);

    if(!addr)
      return EHOSTUNREACH;

    m_d->m_server.sin_addr.s_addr = addr->s_addr;

    return 0;
  }

  bool UDPSocket::close()
  {
    if(m_d->m_fd < 0)
      return false;

    ::close(m_d->m_fd);

    m_d->m_fd = -1;

    return true;
  }

  bool UDPSocket::isOpen() const
  {
    return (m_d->m_fd > 0);
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitfordata)
  {
    if(m_d->m_fd < 0) 
      return -1;

    if(waitfordata) 
      error("UDPSocket::read # waitfordata not yet supported for UDP sockets.");
    else {
      
      struct pollfd pfd;
      bzero( & pfd, sizeof(pfd));

      pfd.fd = m_d->m_fd;
      pfd.events = POLLIN;
      poll(&pfd, 1, 0);
      
      if(!pfd.revents & POLLIN)
	return 0;
    }

    struct sockaddr_in from;
    socklen_t l = sizeof(from);
    return recvfrom(m_d->m_fd, buffer, bytes, 0,
                    (struct sockaddr *) & from,
                    & l);
  }

  int UDPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0)
      return -1;

    return sendto(m_d->m_fd, buffer,
                  bytes, 0, (const sockaddr*) & m_d->m_server, 
                  sizeof(m_d->m_server));
  }

}
