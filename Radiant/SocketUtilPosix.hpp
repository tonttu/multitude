/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SOCKETUTILPOSIX_HPP
#define RADIANT_SOCKETUTILPOSIX_HPP

///@cond

#include "Export.hpp"
#include "SocketWrapper.hpp"
#include <QString>

namespace Radiant {

  class RADIANT_API SocketUtilPosix
  {
  public:
    /// Binds or connects to the socket
    /// @param[out] Reference to file descriptor
    /// @param host Name of the host
    /// @param port Port number
    /// @param errstr[out] In case of error, the error message is stored in this
    /// @param doBind Do we try to bind to socket
    /// @param family Protocol family for socket
    /// @param socktype Socket type
    /// @param protocol Protocol for the socket
    /// @param flags Input flags
    static int bindOrConnectSocket(int & fd, const char * host, int port,
                                   QString & errstr, bool doBind,
                                   int family = AF_UNSPEC, int socktype = 0,
                                   int protocol = 0, int flags = 0);
  };
}

///@endcond

#endif // SOCKETUTILPOSIX_HPP
