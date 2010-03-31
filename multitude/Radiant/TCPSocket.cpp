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

#include "TCPSocket.hpp"

#ifndef WIN32
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#include <WinPort.h>
#endif

namespace Radiant {

  /* Converts ascii text to in_addr struct.  NULL is returned if the address
     can not be found. */
  struct in_addr * TCPSocket::atoaddr(const char *address)

  {
    struct hostent *host;
    static struct in_addr saddr;
    
    /* First try it as aaa.bbb.ccc.ddd. */
    saddr.s_addr = inet_addr(address);
    if ((int) saddr.s_addr != -1) {
      return &saddr;
    }
    host = gethostbyname(address);
    if (host != NULL) {
      return (struct in_addr *) *host->h_addr_list;
    }
    return NULL;
  }

} 
