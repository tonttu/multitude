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

// Yes, this has to be on the top.
#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#endif

#define IGNORE_MULTITUDE_TIMEVAL 1

#include "TCPSocket.hpp"
#include "Thread.hpp"

#include "Sleep.hpp"
#include "Trace.hpp"

#include <QTcpSocket>

#include <errno.h>




namespace Radiant
{

  class TCPSocket::D : public QTcpSocket
  {};

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  TCPSocket::TCPSocket()
  {
    m_d = new D();
  }

  TCPSocket::TCPSocket(int fd)
  {
    m_d = new D();
    if(!m_d->setSocketDescriptor(fd))
      Radiant::error("TCPSocket::TCPSocket # setSocketDescriptor failed");
  }

  TCPSocket::~TCPSocket()
  {
    delete m_d;
  }

  bool TCPSocket::setNoDelay(bool noDelay)
  {
    int yes = noDelay;

    if (setsockopt(m_d->socketDescriptor (),
		   6, 1, (char *) & yes, 
		   sizeof(int))) {
      // 6 = TCP
      // 1 = NODELAY
      error("Could not set option TCP_NODELAY");
      return false;
    }

    return true;
  }

  int TCPSocket::open(const char * host, int port)
  {
    close();

    m_d->connectToHost(host, port);

    bool ok = m_d->waitForConnected(5000);

    if(!ok) {
      QString errstr = m_d->errorString() ;
      error("TCPSocket::open # %s", errstr.toStdString().c_str());
      return EINVAL;
    }

    setNoDelay(true);
    
    return 0;
  }

  bool TCPSocket::close()
  {
    int count = 0;

    while(m_d->bytesToWrite() && count < 500) {
      Radiant::Sleep::sleepMs(5);
      count++;
    }

    m_d->close();

    return true;
  }
 
  bool TCPSocket::isOpen() const
  {
    return m_d->isValid();
  }

  const char * TCPSocket::host() const
  {
    return m_d->peerName().toAscii();
  }

  int TCPSocket::port() const
  {
    return m_d->peerPort();
  }

  int TCPSocket::read(void * buffer, int bytes, bool waitfordata)
  {
    int got = 0;
    char * ptr = (char *) buffer;
    int loops = 0;

    while((got < bytes) && (m_d->state() == QAbstractSocket::ConnectedState)) {

      m_d->waitForReadyRead(1);

      int n = m_d->read(ptr + got, bytes - got);
      got += n;
      loops++;

      if(!waitfordata) {
	return got;
      }
    }
    /*
    info("TCPSocket::read # %d/%d state = %d %d",
	 got, bytes, (int)  m_d->state(), loops);
    */
    return got;
  }

  int TCPSocket::write(const void * buffer, int bytes)
  {
    int n = m_d->write((const char *)buffer, bytes);

    // if(n == bytes)
    m_d->flush();

    return n;
  }

  bool TCPSocket::isHungUp() const
  {
    return (m_d->state() != QAbstractSocket::ConnectedState);
  }

  bool TCPSocket::isPendingInput(unsigned int waitMicroSeconds)
  {
    return m_d->waitForReadyRead(waitMicroSeconds / 1000);
  }

  void TCPSocket::debug()
  {
    Radiant::info("TCPSocket::debug #");
    Radiant::info("\tSTATE %d", m_d->state());
    Radiant::info("\tVALID %d", m_d->isValid());
    Radiant::info("\tERROR %d", m_d->error());
  }

  void TCPSocket::moveToThread(Thread * t) {
    QThread * qt = t->qtThread();
    if(qt) m_d->moveToThread(qt);
  }

}
