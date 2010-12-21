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
#include "Trace.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

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
    : m_d(new D)
  {
  }
  
  TCPServerSocket::~TCPServerSocket()
  {
    close();
    delete m_d;
  }

  int TCPServerSocket::open(const char * host, int port, int maxconnections)
  {
    close();

    m_d->m_host = host ? host : "";
    m_d->m_port = port;

    errno = 0;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0) {
      int err = errno;
      error("TCPServerSocket::open # Failed to open TCP socket: %s", strerror(err));
      return err;
    }

    struct addrinfo hints;
    struct addrinfo * result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char service[32];
    sprintf(service, "%d", port);

    int s = getaddrinfo(host && *host ? host : 0, service, &hints, &result);
    if(s) {
      error("TCPServerSocket::open # getaddrinfo: %s", gai_strerror(s));
      ::close(fd);
      return -1;
    }

    struct addrinfo * rp;
    for(rp = result; rp; rp = rp->ai_next)
      if(bind(fd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;

    freeaddrinfo(result);

    if(rp == NULL) {
      error("TCPServerSocket::open # Failed to bind %s:%d", host ? host : "", port);
      ::close(fd);
      return -1;
    }

    errno = 0;
    if(::listen(fd, maxconnections) != 0) {
      int err = errno;
      error("TCPServerSocket::open # Failed to listen TCP socket: %s", strerror(err));
      ::close(fd);
      return err;
    }

    m_d->m_fd = fd;

    return 0;
  }

  bool TCPServerSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;

    if(shutdown(fd, SHUT_RDWR)) {
      error("TCPServerSocket::close # Failed to shut down the socket: %s", strerror(errno));
    }
    if(::close(fd)) {
      error("TCPServerSocket::close # Failed to close socket: %s", strerror(errno));
    }

    return true;
  }

  bool TCPServerSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
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
    if(m_d->m_fd < 0)
      return 0;

    sockaddr newAddress;
    socklen_t addressLength(sizeof(newAddress));

    bzero( & newAddress, sizeof(newAddress));

    for(;;) {
      errno = 0;
      int fd = ::accept(m_d->m_fd, (sockaddr *) & newAddress, & addressLength);

      if(fd >= 0)
        return new TCPSocket(fd);

      if(fd < 0) {
        if(m_d->m_fd == -1)
          return 0;
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          poll(&pfd, 1, 5000);
        } else {
          error("TCPServerSocket::accept # %s", strerror(errno));
          return 0;
        }
      }
    }

    return 0;
  }
}

