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
#include <string.h>
#include <iostream>
#include <stdio.h>

namespace Radiant
{

  class TCPSocket::D {
    public:
      D(int fd = -1)
        : m_fd(fd),
        m_port(0),
        m_noDelay(0)
      {}

      bool setOpts()
      {
        if(m_fd < 0) return true;

        bool ok = true;

        if(setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &m_noDelay, sizeof(m_noDelay))) {
          ok = false;
          error("Failed to set TCP_NODELAY: %s", strerror(errno));
        }

        return ok;
      }

      int m_fd;
      int m_port;
      int m_noDelay;
      std::string m_host;
  };

 ////////////////////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////

  TCPSocket::TCPSocket() : m_d(new D)
  {
  }

  TCPSocket::TCPSocket(int fd) : m_d(new D(fd))
  {
    m_d->setOpts();
  }

  TCPSocket::~TCPSocket()
  {
    close();
    delete m_d;
  }

  bool TCPSocket::setNoDelay(bool noDelay)
  {
    m_d->m_noDelay = noDelay;
    return m_d->setOpts();
  }

  int TCPSocket::open(const char * host, int port)
  {
    close();

    m_d->m_host = host;
    m_d->m_port = port;

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0) {
      int err = errno;
      error("TCPSocket::open # Failed to open TCP socket: %s", strerror(err));
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

    int s = getaddrinfo(host, service, &hints, &result);
    if(s) {
      error("TCPSocket::open # getaddrinfo: %s", gai_strerror(s));
      ::close(fd);
      return -1;
    }

    struct addrinfo * rp;
    for(rp = result; rp; rp = rp->ai_next)
      if(connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;

    freeaddrinfo(result);

    if(rp == NULL) {
      error("TCPSocket::open # Failed to connect %s:%d", host, port);
      ::close(fd);
      return -1;
    }

    m_d->m_fd = fd;
    m_d->setOpts();

    return 0;
  }

  bool TCPSocket::close()
  {
    if(m_d->m_fd < 0)
      return false;

    if(::close(m_d->m_fd)) {
      error("TCPSocket::close # Failed to close socket: %s", strerror(errno));
    }

    m_d->m_fd = -1;

    return true;
  }

  bool TCPSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
  }

  int TCPSocket::read(void * buffer, int bytes, bool waitfordata)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    int pos = 0;
    char * data = reinterpret_cast<char*>(buffer);

    while(pos < bytes) {
      errno = 0;
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      int tmp = ::read(m_d->m_fd, data + pos, max);

      if(tmp > 0) {
        pos += tmp;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        if(waitfordata) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          poll(&pfd, 1, 5000);
        } else {
          return pos;
        }
      } else {
        error("TCPSocket::readExact # Failed to read: %s", strerror(errno));
        return pos;
      }
    }

    return pos;
  }

  int TCPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    int pos = 0;
    const char * data = reinterpret_cast<const char*>(buffer);

    while(pos < bytes) {
      errno = 0;
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      int tmp = ::write(m_d->m_fd, data + pos, max);
      if(tmp > 0) {
        pos += tmp;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd pfd;
        pfd.fd = m_d->m_fd;
        pfd.events = POLLOUT;
        poll(&pfd, 1, 5000);
      } else {
        error("TCPSocket::write # Failed to write: %s", strerror(errno));
        return pos;
      }
    }

    return pos;
  }

  bool TCPSocket::isHungUp() const
  {
    if(m_d->m_fd < 0)
      return true;

    struct pollfd pfd;
    bzero( & pfd, sizeof(pfd));
    pfd.fd = m_d->m_fd;
    pfd.events = ~0;
    poll(&pfd, 1, 0);

   return (pfd.revents & (POLLHUP)) != 0;
   // return (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
  }

  bool TCPSocket::isPendingInput(unsigned int waitMicroSeconds)
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

  void TCPSocket::moveToThread(Thread *) {
  }

}

