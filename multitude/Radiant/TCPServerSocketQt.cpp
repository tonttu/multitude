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

#include <QTcpServer>
#include <QTcpSocket>

#include <Radiant/Trace.hpp>

#include <errno.h>

namespace Radiant
{

  class TCPServerSocket::D : public QTcpServer
  {
  public:
    D() : QTcpServer() {}

    int pendingConnection() {
      if(m_fds.empty()) return -1;

      int fd = m_fds.front();
      m_fds.pop_front();

      return fd;
    }

  protected:
    virtual void incomingConnection(int socketDescriptor) {
      m_fds.push_back(socketDescriptor);
    }
  
    std::list<int> m_fds;
  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  TCPServerSocket::TCPServerSocket()
  {
    m_d = new D();
  }


  TCPServerSocket::~TCPServerSocket()
  {
    delete m_d;
  }

  int TCPServerSocket::open(const char * host, int port, int maxconnections)
  {
    //Radiant::info("TCPServerSocket::open # open(%s, %d, %d)", host, port, maxconnections);

    char h[130];
    if(strcmp(host, "localhost") != 0)
      strcpy(h, host);
    else
      strcpy(h, "127.0.0.1");

    bool r = m_d->listen(QHostAddress(h), port);

    if(!r)
      error("TCPServerSocket::open # %s:%d (%s)", h, port,
	    m_d->errorString().toStdString().c_str());

    m_d->setMaxPendingConnections(maxconnections);

    return r ? 0 : EINVAL;
  }

  bool TCPServerSocket::close() 
  {
    m_d->close();

    return true;
  }

  bool TCPServerSocket::isOpen() const
  {
    return m_d->isListening();
  }
 
  bool TCPServerSocket::isPendingConnection(unsigned int waitMicroSeconds)
  {
    return m_d->waitForNewConnection(waitMicroSeconds / 1000);
  }

  TCPSocket * TCPServerSocket::accept()
  {
    int fd = m_d->pendingConnection();
    if(fd == -1) return 0;

    TCPSocket * tcp = new TCPSocket(fd);

    QTcpSocket * qtcp = (QTcpSocket*) tcp->m_d;

    if(!qtcp->waitForConnected()) {
      error("TCPServerSocket::accept # Could not connect %s",
	    qtcp->errorString().toStdString().c_str());
      delete tcp;
      return 0;
    }

    return tcp;
  }
  
}

