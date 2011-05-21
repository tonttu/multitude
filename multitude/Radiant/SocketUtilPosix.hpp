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
