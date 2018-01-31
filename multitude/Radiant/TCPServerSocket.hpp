/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TCP_SERVER_SOCKET_HPP
#define RADIANT_TCP_SERVER_SOCKET_HPP

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Export.hpp>

#include <QString>

namespace Radiant {

  class TCPSocket;

  /// A server TCP socket for accepting incoming connections
  /// @todo Example code
  class RADIANT_API TCPServerSocket : public Patterns::NotCopyable
  {
  public:
    /// Constructor
    TCPServerSocket();
    /// Destructor
    ~TCPServerSocket();

    /// Opens a server TCP socket to desired host:port
    /// @param host Hostname
    /// @param port Port number
    /// @param maxconnections Maximum number of pending connections
    /// @return On successful execution, returns zero, otherwise an
    ///         error code (as in errno.h).
    int open(const char * host, int port, int maxconnections = 2);

    /// Closes the socket
    /// @return True if the socket was succesfulle closed
    bool close();

    /// Returns true of the socket is open.
    /// @return True if the socket is open, false otherwise
    bool isOpen() const;

    /// Returns the hostname
    /// @return Hostname of the socket
    const QString host() const;
    /// Returns the port number
    /// @return Port number of the socket
    int port() const;

    /// Check for pending connections
    /// Checks for pending connections and optionally blocks for the given timeout.
    /// @param waitMicroSeconds micro seconds to block
    /// @return true if there are pending connections
    /// @todo change the units to milliseconds in 2.1
    bool isPendingConnection(unsigned int waitMicroSeconds = 0);

    /// Accepts new connections. Blocks until a connection is received or the socket is closed.
    /// @return connected socket or null in case of error
    TCPSocket * accept();

    /// Moves the wrapped TCP socket and its ownership to the caller. After
    /// calling this, TCPServerSocket::isOpen will return false.
    /// Returns -1 if the socket is not open
    int takeSocket();

    /// Returns the TCP socket, or -1 if the socket is not open
    int socket() const;

  private:
    class D;
    D * m_d;
  };

}

#endif
