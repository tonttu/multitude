/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef SOCKETWRAPPER_HPP
#define SOCKETWRAPPER_HPP

/// @cond

#include "Export.hpp"

#ifdef RADIANT_WINDOWS
#include <winsock2.h>
#include <WS2tcpip.h>
#include <cerrno>

#ifndef EADDRINUSE

#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
//#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
//#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE

#endif

#define SHUT_RDWR SD_BOTH

namespace SocketWrapper
{
  inline int close(int fd) { return closesocket(fd); }
  inline int poll(struct pollfd * fds, unsigned long nfds, int timeout) { return WSAPoll(fds, nfds, timeout); }
  RADIANT_API const char * strerror(int errnum);
  // errno seems to be a macro in GCC, can't have function named errno..
  inline int err() { return WSAGetLastError(); }
  inline void clearErr() {}
  inline const char * gai_strerror(int errcode) { return ::gai_strerrorA(errcode); }
  RADIANT_API void startup();
}

#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace SocketWrapper
{
  inline int close(int fd) { return ::close(fd); }
  inline int poll(struct pollfd * fds, nfds_t nfds, int timeout) { return ::poll(fds, nfds, timeout); }
  inline char * strerror(int errnum) { return ::strerror(errnum); }
  // errno seems to be a macro in GCC, can't have function named errno..
  inline int err() { return errno; }
  inline void clearErr() { errno = 0; }
  inline const char * gai_strerror(int errcode) { return ::gai_strerror(errcode); }
  inline void startup() {}
}

#endif

/// @endcond

#endif // SOCKETWRAPPER_HPP
