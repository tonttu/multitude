/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
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
    static int bindOrConnectSocket(int & fd, const char * host, int port,
                                   QString & errstr, bool doBind,
                                   int family = AF_UNSPEC, int socktype = 0,
                                   int protocol = 0, int flags = 0);
  };
}

///@endcond

#endif // SOCKETUTILPOSIX_HPP
