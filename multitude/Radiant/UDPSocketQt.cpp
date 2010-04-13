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
#include "Trace.hpp"

#include <QUdpSocket>

namespace Radiant
{

  class UDPSocket::D : public QUdpSocket
  {};

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

  bool UDPSocket::bind(const std::string &address, uint16_t port)
  {
      return m_d->bind(QHostAddress(address.c_str()), port);
  }

  int UDPSocket::readDatagram(char *data, size_t maxSize, std::string *fromAddr, uint16_t *fromPort)
  {
      QHostAddress from;

      int result = m_d->readDatagram(data, maxSize, (fromAddr ? &from : 0), fromPort);

      if(fromAddr)
          *fromAddr = from.toString().toStdString();

      return result;
  }

  int UDPSocket::writeDatagram(const char *data, size_t bytes, const std::string & addr, uint16_t port)
  {
      return m_d->writeDatagram(data, bytes, QHostAddress(addr.c_str()), port);
  }
 
}

