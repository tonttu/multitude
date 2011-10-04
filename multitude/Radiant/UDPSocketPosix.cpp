/* COPYRIGHT
 */

#include "UDPSocket.hpp"
#include "SocketWrapper.hpp"
#include "SocketUtilPosix.hpp"
#include "Trace.hpp"

#include <errno.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>

namespace Radiant
{

  class UDPSocket::D {
  public:
    D(int fd = -1) : m_fd(fd), m_port(0)
    {
    }

    int m_fd;
    int m_port;
    QString m_host;
  };

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

  UDPSocket::UDPSocket() : m_d(new D)
  {
    wrap_startup();
  }

  UDPSocket::UDPSocket(int fd) : m_d(new D(fd))
  {
    wrap_startup();
  }

  UDPSocket::~UDPSocket()
  {
    close();
    delete m_d;
  }

  int UDPSocket::openServer(int port)
  {
    close();

    m_d->m_host.clear();
    m_d->m_port = port;

    QString errstr;
    int err = SocketUtilPosix::bindOrConnectSocket(m_d->m_fd, "0.0.0.0", port, errstr,
                  true, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(err) {
      error("UDPSocket::open # Failed to bind to port %d: %s", port, errstr.toUtf8().data());
    }

    return err;
  }

  int UDPSocket::openClient(const char * host, int port)
  {
    close();

    m_d->m_host = host;
    m_d->m_port = port;

    QString errstr;
    int err = SocketUtilPosix::bindOrConnectSocket(m_d->m_fd, host, port, errstr,
                  false, AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(err) {
      error("UDPSocket::openClient # Failed to connect %s:%d: %s", host, port, errstr.toUtf8().data());
    }

    return err;
  }

  bool UDPSocket::close()
  {
    int fd = m_d->m_fd;
    if(fd < 0)
      return false;

    m_d->m_fd = -1;

    if(!m_d->m_host.isEmpty() && shutdown(fd, SHUT_RDWR)) {
      error("UDPSocket::close # Failed to shut down the socket: %s", wrap_strerror(wrap_errno));
    }
    if(wrap_close(fd)) {
      error("UDPSocket::close # Failed to close socket: %s", wrap_strerror(wrap_errno));
    }

    return true;
  }

  bool UDPSocket::isOpen() const
  {
    return m_d->m_fd >= 0;
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitfordata = false)
  {
    return read(buffer, bytes, waitfordata, false);
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitfordata, bool readAll)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    int pos = 0;
    char * data = reinterpret_cast<char*>(buffer);

#ifdef RADIANT_WINDOWS
    // Windows doesn't implement MSG_DONTWAIT, so do an extra poll
    if(!waitfordata && !readAll){
      struct pollfd pfd;
      pfd.fd = m_d->m_fd;
      pfd.events = POLLIN;
      if(wrap_poll(&pfd, 1, 0) <= 0 || (pfd.revents & POLLIN) == 0)
        return 0;
    }
    int flags = 0;
#else
    int flags = (readAll || waitfordata) ? 0 : MSG_DONTWAIT;
#endif

    while(pos < bytes) {
      errno = 0;
      // int max = bytes - pos > SSIZE_MAX ? SSIZE_MAX : bytes - pos;
      int max = bytes - pos > 32767 ? 32767 : bytes - pos;
      /// @todo should we care about the sender?
      int tmp = recv(m_d->m_fd, data + pos, max, flags);

      if(tmp > 0) {
        pos += tmp;
        if(!readAll) return pos;
      } else if(tmp == 0 || m_d->m_fd == -1) {
        return pos;
      } else if(errno == EAGAIN || errno == EWOULDBLOCK) {
        if(readAll || (waitfordata && pos == 0)) {
          struct pollfd pfd;
          pfd.fd = m_d->m_fd;
          pfd.events = POLLIN;
          wrap_poll(&pfd, 1, 5000);
        } else {
          return pos;
        }
      } else {
        error("UDPSocket::read # Failed to read: %s", wrap_strerror(wrap_errno));
        return pos;
      }
    }

    return pos;
  }

  int UDPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0 || bytes < 0)
      return -1;

    if(m_d->m_host.isEmpty()) {
      /// @todo implement writeTo() or something similar
      error("UDPSocket::write # This socket was created using openServer, "
            "it's not connected. Use writeTo() instead.");
      return -1;
    }

    int pos = 0;
    const char * data = reinterpret_cast<const char*>(buffer);

    while(pos < bytes) {
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
        error("UDPSocket::write # Failed to write: %s", wrap_strerror(wrap_errno));
        return pos;
      }
    }

    return pos;
  }

  bool UDPSocket::setReceiveBufferSize(size_t bytes)
  {
    if(m_d->m_fd < 0)
      return false;

    int n = static_cast<int> (bytes);

    if (setsockopt(m_d->m_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&n, sizeof(n)) == -1) {
      return false;
    }
    return true;
  }
}
