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
#include "SocketUtilPosix.hpp"
#include "SocketWrapper.hpp"
#include "Trace.hpp"

#include <errno.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>

namespace Radiant
{

  class TCPServerSocket::D {
    public:
      D() : m_fd(-1), m_port(0) {}

      int m_fd;
      int m_port;
      QString m_host;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  TCPServerSocket::TCPServerSocket()
    : m_d(new D)
  {
    wrap_startup();
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

    QString errstr;
    int fd = -1;
    int err = SocketUtilPosix::bindOrConnectSocket(fd, host, port, errstr,
                  true, AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(err) {
      error("TCPServerSocket::open # %s", errstr.c_str());
      return err;
    }

    if(::listen(fd, maxconnections) != 0) {
      int err = wrap_errno;
      error("TCPServerSocket::open # Failed to listen TCP socket: %s", wrap_strerror(err));
      wrap_close(fd);
      return err ? err : -1;
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
      error("TCPServerSocket::close # Failed to shut down the socket: %s", wrap_strerror(wrap_errno));
    }
    if(wrap_close(fd)) {
      error("TCPServerSocket::close # Failed to close socket: %s", wrap_strerror(wrap_errno));
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
    wrap_poll(&pfd, 1, waitMicroSeconds / 1000);

    return (pfd.revents & POLLIN) == POLLIN;
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
        int err = wrap_errno;
        if(err == EAGAIN || err == EWOULDBLOCK) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          wrap_poll(&pfd, 1, 5000);
        } else {
          error("TCPServerSocket::accept # %s", wrap_strerror(wrap_errno));
          return 0;
        }
      }
    }

    return 0;
  }
}

