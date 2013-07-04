/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Platform.hpp"
#include "TCPSocket.hpp"
#include "SocketUtilPosix.hpp"
#include "SocketWrapper.hpp"
#include "Trace.hpp"

#include <sys/types.h>

#include "signal.h"

namespace Radiant
{

  class TCPSocket::D {
    public:
      D(int fd = -1)
        : m_fd(fd),
        m_port(0),
        m_noDelay(0),
        m_rxBytes(0),
        m_txBytes(0)
      {}

      bool setOpts()
      {
        if(m_fd < 0) return true;

        bool ok = true;

        if(setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (const char*)&m_noDelay, sizeof(m_noDelay))) {
          ok = false;
          error("Failed to set TCP_NODELAY: %s", SocketWrapper::strerror(SocketWrapper::err()));
        }

        return ok;
      }

      int m_fd;
      int m_port;
      int m_noDelay;
      unsigned long m_rxBytes, m_txBytes;
      QString m_host;
  };

 ////////////////////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////

  TCPSocket::TCPSocket() : m_d(new D)
  {
    /// @todo this should probably moved elsewhere. No reason to run this multiple times.
    // ignore SIGPIPE (ie. when client closes the socket)
  #ifndef _MSC_VER
    signal(SIGPIPE, SIG_IGN);
  #endif

    SocketWrapper::startup();
  }

  TCPSocket::TCPSocket(int fd) : m_d(new D(fd))
  {
    SocketWrapper::startup();
    m_d->setOpts();
  }

  TCPSocket::~TCPSocket()
  {
    if (m_d) {
      close();
      delete m_d;
    }
  }

  TCPSocket::TCPSocket(TCPSocket && socket)
    : m_d(socket.m_d)
  {
    socket.m_d = nullptr;
  }

  TCPSocket & TCPSocket::operator=(TCPSocket && socket)
  {
    std::swap(m_d, socket.m_d);
    return *this;
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
    m_d->m_rxBytes = m_d->m_txBytes = 0;

    QString errstr;
    int err = SocketUtilPosix::bindOrConnectSocket(m_d->m_fd, host, port, errstr,
                  false, AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(err == 0) {
      m_d->setOpts();
    } else {
      error("TCPSocket::open(%s:%d) # %s", host, port, errstr.toUtf8().data());
    }
    return err;
  }

  bool TCPSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;

    // ignore error if the connection closed in abortive way
    shutdown(fd, SHUT_RDWR);

    if(SocketWrapper::close(fd)) {
      error("TCPSocket::close # Failed to close socket: %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return true;
  }

  bool TCPSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
  }

  int TCPSocket::read(void * buffer, int bytes, Flags flags)
  {
    if(m_d->m_fd < 0) {
      Radiant::error("TCPSocket::read # invalid socket (file descriptor = %d)", m_d->m_fd);
      return -1;
    }

    if(bytes < 1) {
      Radiant::error("TCPSocket::read # bytes to read must be >= 1 (was %d)", bytes);
      return -1;
    }

    int pos = 0;
    char * data = reinterpret_cast<char*>(buffer);

    while(pos < bytes) {
      SocketWrapper::clearErr();
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      bool block = flags == WAIT_ALL || (flags == WAIT_SOME && pos == 0);
      int tmp;

      if(block) {
        tmp = recv(m_d->m_fd, data + pos, max, 0);
      } else {
#ifdef RADIANT_WINDOWS
        // there is no MSG_DONTWAIT in winsock, do poll with zero timeout instead
        if(!isPendingInput()) return pos;
        tmp = recv(m_d->m_fd, data + pos, max, 0);
#else
        tmp = recv(m_d->m_fd, data + pos, max, MSG_DONTWAIT);
#endif
      }

      if(tmp > 0) {
        pos += tmp;
        m_d->m_rxBytes += tmp;
      } else if(tmp == 0 || m_d->m_fd == -1) {
        return pos;
      } else if(SocketWrapper::err() == EAGAIN || SocketWrapper::err() == EWOULDBLOCK) {
        if(!block) return pos;

        struct pollfd pfd;
        pfd.fd = m_d->m_fd;
        pfd.events = POLLIN;
        SocketWrapper::poll(&pfd, 1, 5000);
      } else {
        error("TCPSocket::read # Failed to read: %s", SocketWrapper::strerror(SocketWrapper::err()));
        return pos;
      }
    }

    return pos;
  }

  int TCPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0) {
      Radiant::error("TCPSocket::write # invalid socket (file descriptor = %d)", m_d->m_fd);
      return -1;
    }

    if(bytes < 1) {
      Radiant::error("TCPSocket::write # bytes to write must be >= 1 (was %d)", bytes);
      return -1;
    }

    int pos = 0;
    const char * data = reinterpret_cast<const char*>(buffer);

    while(pos < bytes) {
      SocketWrapper::clearErr();
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      int tmp = send(m_d->m_fd, data + pos, max, 0);
      if(tmp > 0) {
        pos += tmp;
        m_d->m_txBytes += tmp;
      } else if(SocketWrapper::err() == EAGAIN || SocketWrapper::err() == EWOULDBLOCK) {
        struct pollfd pfd;
        pfd.fd = m_d->m_fd;
        pfd.events = POLLOUT;
        SocketWrapper::poll(&pfd, 1, 5000);
      } else {
        error("TCPSocket::write # Failed to write: %s", SocketWrapper::strerror(SocketWrapper::err()));
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
    memset( & pfd, 0, sizeof(pfd));
    pfd.fd = m_d->m_fd;
    pfd.events = POLLWRNORM;
    int status = SocketWrapper::poll(&pfd, 1, 0);
    if(status == -1) {
      Radiant::error("TCPSocket::isHungUp %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
  }

  bool TCPSocket::isPendingInput(unsigned int waitMicroSeconds)
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    memset( & pfd, 0, sizeof(pfd));

    pfd.fd = m_d->m_fd;
    pfd.events = POLLRDNORM;
    int status = SocketWrapper::poll(&pfd, 1, waitMicroSeconds / 1000);
    if(status == -1) {
      Radiant::error("TCPSocket::isPendingInput %s", SocketWrapper::strerror(SocketWrapper::err()));
    }

    return (pfd.revents & POLLRDNORM) == POLLRDNORM;
  }

  void TCPSocket::moveToThread(Thread *)
  {
  }

  int TCPSocket::fd() const
  {
    return m_d->m_fd;
  }

  unsigned long TCPSocket::rxBytes() const
  {
    return m_d->m_rxBytes;
  }

  unsigned long TCPSocket::txBytes() const
  {
    return m_d->m_txBytes;
  }

  const QString& TCPSocket::host() const
  {
    return m_d->m_host;
  }

  int TCPSocket::port() const
  {
    return m_d->m_port;
  }
}

