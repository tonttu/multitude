/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "SocketUtilPosix.hpp"
#include "SocketWrapper.hpp"
#include "Trace.hpp"

#include <string.h>
#include <stdio.h>

#ifdef RADIANT_WINDOWS
const char * SocketWrapper::strerror(int err)
{
  __declspec( thread ) static char msg[1024];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                FORMAT_MESSAGE_MAX_WIDTH_MASK, 0, err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)msg, 1024, 0);
  return msg;
}

void SocketWrapper::startup()
{
  static bool s_ready = false;

  if(s_ready) return;

  WORD version = MAKEWORD(2, 0);
  WSADATA data;

  /// @todo should we care about WSACleanup()
  int err = WSAStartup(version, &data);
  if(err != 0) {
    Radiant::error("WSAStartup failed with error: %d", err);
    return;
  }
  s_ready = true;
}
#endif

namespace Radiant {

  int SocketUtilPosix::bindOrConnectSocket(int & fd, const char * host, int port,
                                           QString & errstr, bool doBind,
                                           int family, int socktype,
                                           int protocol, int flags)
  {
    SocketWrapper::startup();
    struct addrinfo hints;
    struct addrinfo * result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;

    char service[32];
    sprintf(service, "%d", port);

    int s = getaddrinfo(host, service, &hints, &result);
    if(s) {
      errstr = QString("getaddrinfo: ") + SocketWrapper::gai_strerror(s);
      return -1;
    }

    int err = 0;
    for(struct addrinfo * rp = result; rp; rp = rp->ai_next) {
      fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

      if(fd < 0) {
        int err = SocketWrapper::err();
        errstr = QString("Failed to open socket: ") + SocketWrapper::strerror(err);
        continue;
      }

      int t = 1;
      if (doBind && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&t, sizeof(t)) < 0) {
        error("TCPServerSocket::open # Failed to set SO_REUSEADDR: %s", SocketWrapper::strerror(SocketWrapper::err()));
      }

      if(doBind && bind(fd, rp->ai_addr, static_cast<int> (rp->ai_addrlen)) == -1) {
        err = SocketWrapper::err();
        errstr = QString("bind(%2, %1) failed: %3").arg(port).arg(host, SocketWrapper::strerror(err));
        err = err ? err : -1;
      } else if(!doBind && connect(fd, rp->ai_addr, static_cast<int> (rp->ai_addrlen)) == -1) {
        err = SocketWrapper::err();
        errstr = QString("connect() failed: ") + SocketWrapper::strerror(err);
        err = err ? err : -1;
      } else {
        err = 0;
        break;
      }

      SocketWrapper::close(fd);
      fd = -1;
    }

    freeaddrinfo(result);

    return err;
  }
}
