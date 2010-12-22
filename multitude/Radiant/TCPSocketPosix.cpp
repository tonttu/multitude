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
#include "SocketWrapper.hpp"
#include "Trace.hpp"

#include <errno.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <iostream>
#include <stdio.h>

#ifdef RADIANT_WIN32
const char * wrap_strerror(int err)
{
  __declspec( thread ) static char msg[1024];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)msg, 1024, 0);
  return msg;
}

void wrap_startup()
{
  static bool s_ready = false;

  if(s_ready) return;

  WORD version = MAKEWORD(2, 0);
  WSADATA data;

  /// @todo should we care about WSACleanup()
  int err = WSAStartup(version, &data);
  if(err != 0) {
    Radiant::error("WSAStartup failed with error: %d", err);
    return;
  }
  s_ready = true;
}

#endif

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

        if(setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&m_noDelay, sizeof(m_noDelay))) {
          ok = false;
          error("Failed to set TCP_NODELAY: %s", wrap_strerror(wrap_errno));
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
    wrap_startup();
  }

  TCPSocket::TCPSocket(int fd) : m_d(new D(fd))
  {
    wrap_startup();
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

    m_d->m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_d->m_fd < 0) {
      int err = wrap_errno;
      error("TCPSocket::open # Failed to open TCP socket: %s", wrap_strerror(err));
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
      wrap_close(m_d->m_fd);
      m_d->m_fd = -1;
      return -1;
    }

    struct addrinfo * rp;
    for(rp = result; rp; rp = rp->ai_next)
      if(connect(m_d->m_fd, rp->ai_addr, rp->ai_addrlen) != -1)
        break;

    freeaddrinfo(result);

    if(rp == NULL) {
      error("TCPSocket::open # Failed to connect %s:%d", host, port);
      wrap_close(m_d->m_fd);
      m_d->m_fd = -1;
      return -1;
    }

    m_d->setOpts();

    return 0;
  }

  bool TCPSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;

    if(shutdown(fd, SHUT_RDWR)) {
      error("TCPSocket::close # Failed to shut down the socket: %s", wrap_strerror(wrap_errno));
    }
    if(wrap_close(fd)) {
      error("TCPSocket::close # Failed to close socket: %s", wrap_strerror(wrap_errno));
    }

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
      int tmp = recv(m_d->m_fd, data + pos, max, 0);

      if(tmp > 0) {
        pos += tmp;
      } else if(tmp == 0 || m_d->m_fd == -1) {
        return pos;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        if(waitfordata) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          wrap_poll(&pfd, 1, 5000);
        } else {
          return pos;
        }
      } else {
        error("TCPSocket::read # Failed to read: %s", wrap_strerror(wrap_errno));
        return pos;
      }
    }

    return pos;
  }

  int TCPSocket::readSome(void * buffer, int bytes, bool waitfordata)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    if(bytes > 32767)
      bytes = 32767;

    for(;;) {
      errno = 0;
      int tmp = recv(m_d->m_fd, (char*)buffer, bytes, 0);

      if(tmp > 0) {
        return tmp;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        if(waitfordata) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          wrap_poll(&pfd, 1, 5000);
        } else {
          return 0;
        }
      } else {
        error("TCPSocket::readSome # Failed to read: %s", wrap_strerror(wrap_errno));
        return 0;
      }
    }

    return 0;
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
      int tmp = send(m_d->m_fd, data + pos, max, 0);
      if(tmp > 0) {
        pos += tmp;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd pfd;
        pfd.fd = m_d->m_fd;
        pfd.events = POLLOUT;
        wrap_poll(&pfd, 1, 5000);
      } else {
        error("TCPSocket::write # Failed to write: %s", wrap_strerror(wrap_errno));
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
    wrap_poll(&pfd, 1, 0);

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
    wrap_poll(&pfd, 1, waitMicroSeconds / 1000);

    return (pfd.revents & POLLIN) == POLLIN;
  }

  void TCPSocket::moveToThread(Thread *) {
  }

}

