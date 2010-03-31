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

  
  int UDPSocket::open(const char * host, int port, bool client)
  {
    if(client)
      return openClient(host, port);
    else
      return openServer(host, port);
  }

  int UDPSocket::openClient(const char * host, int port)
  {
    close();

    // m_d->bind(QHostAddress(QString(host)), port);
    m_d->connectToHost(host, port);
	if(!m_d->waitForConnected())
		return 1;

    return 0; 
  }

  int UDPSocket::openServer(const char * host, int port)
  {
    close();

    if(!m_d->bind(QHostAddress(QString(host)), port))
	  return 1;
    //m_d->connectToHost(host, port);

    return 0; 
  }

  bool UDPSocket::close()
  {
    m_d->close();

    return true;
  }

  bool UDPSocket::isOpen() const
  {
    return m_d->isValid();
  }

  static inline bool __stateOk(QAbstractSocket::SocketState s)
  {
	return (s == QAbstractSocket::ConnectedState) ||
		   (s == QAbstractSocket::BoundState);
  }

  int UDPSocket::read(void * buffer, int bytes, bool waitForData)
  {
    int got = 0;
    char * ptr = (char *) buffer;
    int loops = 0;

    if(!__stateOk(m_d->state())) {
      error("UDPSocket::read # Socket not connected # %d",
	    (int) m_d->state());
      return -1;
    }

    if(waitForData) {
      while((got < bytes) && !__stateOk(m_d->state())) {

        // bool something = m_d->waitForReadyRead(1);
	
        int n = m_d->read(ptr + got, bytes - got);
        got += n;
        loops++;
	
        if(!waitForData) {
	  return got;
        }
      }
    }
    else if(m_d->hasPendingDatagrams()) {
      got = m_d->readDatagram(ptr, bytes);
    }
    
    return got;
  }

  int UDPSocket::write(const void * buffer, int bytes)
  {
    return m_d->write((const char *)buffer, bytes);
  }
 
}

