/* COPYRIGHT
 */

#include "UDPSocket.hpp"
#include "Trace.hpp"

#include <QUdpSocket>
#include "SocketWrapper.hpp"
#include "errno.h"

namespace Radiant
{

  class UDPSocket::D : public QUdpSocket
  {
  public:
    D() : m_port(-1) {}

    QHostAddress m_addr;
    int m_port;
  };

  UDPSocket::UDPSocket()
  {
    m_d = new D();
  }

  UDPSocket::UDPSocket(int fd)
  {
    m_d = new D();
    if(!m_d->setSocketDescriptor(fd))
      Radiant::error("UDPSocket::UDPSocket # failed to set socket descriptor");
  }

  UDPSocket::~UDPSocket()
  {
    delete m_d;
  }

  bool UDPSocket::isOpen() const
  {
    return m_d->isValid();
  }

  bool UDPSocket::close()
  {
    return true;
  }

  int UDPSocket::openServer(int port)
  {
    int ok = m_d->bind(port);
    if(!m_d->isValid())
      ok = false;
    return ok ? 0 : EADDRNOTAVAIL;
  }

  int UDPSocket::openClient(const char * host, int port)
  {
    m_d->m_addr = QHostAddress(host);
    m_d->m_port = port;

    if(m_d->m_addr.isNull())
      return EADDRNOTAVAIL;

    /*
    if(!m_d->isValid())
      return ENOTSOCK;
*/
    return 0;
  }

  int UDPSocket::read(void *data, int maxSize, bool block)
  {
    if(block)
      error("UDPSocket::read # Blocking mode not implemented");

    int result = m_d->readDatagram((char *) data, maxSize);

    return result;
  }

  int UDPSocket::write(const void *data, int bytes)
  {
    return m_d->writeDatagram((const char *) data, bytes, m_d->m_addr, m_d->m_port);
  }

  bool UDPSocket::setReceiveBufferSize(size_t /*bytes*/)
  {
    return false;
  }

}

