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

#ifndef RADIANT_TCP_SERVER_SOCKET_HPP
#define RADIANT_TCP_SERVER_SOCKET_HPP

#include <Radiant/Export.hpp>

#include <string>

namespace Radiant {

  class TCPSocket;

  /// A server TCP socket for accepting incoming connections
  /// @todo Example code

  class RADIANT_API TCPServerSocket
  {
  public:
    TCPServerSocket();
    ~TCPServerSocket();

    /// Opens a server TCP socket to desired host:port
    /** @return On successful execution, returns zero, otherwise an
        error code (as in errno.h). */
    int open(const char * host, int port, int maxconnections = 2);
    /// Closes the socket
    bool close();
    /// Returns true of the socket is open.
    bool isOpen() const;

    const char * host() const;
    int port() const;

    /// Return 'true' if connection pending.
    bool isPendingConnection(unsigned int waitMicroSeconds = 0);

    /// Accept new connections
    TCPSocket * accept();

  private:
    TCPServerSocket(const TCPServerSocket & ) {}
    TCPServerSocket & operator = (const TCPServerSocket & ) { return * this; }

    class D;
    D * m_d;
  };

}

#endif
