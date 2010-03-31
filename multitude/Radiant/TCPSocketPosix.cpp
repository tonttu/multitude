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

#include "Trace.hpp"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <iostream>
namespace Radiant
{

  class TCPSocket::D {
    public:
      D() : m_fd(-1), m_port(0) {}

      int m_fd;
      int m_port;
      std::string m_host;
  };

 ////////////////////////////////////////////////////////////////////////////////
 ////////////////////////////////////////////////////////////////////////////////

  TCPSocket::TCPSocket()
  {
    m_d = new D();
  }

  TCPSocket::TCPSocket(int fd)
  {
    m_d = new D();
    m_d->m_fd = fd;
  }

  TCPSocket::~TCPSocket()
  {
    delete m_d;
  }

  bool TCPSocket::setNoDelay(bool noDelay)
  {
    int yes = noDelay;

    if (setsockopt(m_d->m_fd, IPPROTO_TCP, TCP_NODELAY, (char *) & yes,
           sizeof(int))) {
      return false;
    }

    return true;
  }

  int TCPSocket::open(const char * host, int port)
  {
    close();

    m_d->m_host = host;
    m_d->m_port = port;

    m_d->m_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_d->m_fd < 0)
      return errno;

    struct sockaddr_in server_address;

    bzero( & server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons((short) port);

    in_addr * addr = TCPSocket::atoaddr(host);

    if(!addr)
      return EHOSTUNREACH;

    server_address.sin_addr.s_addr = addr->s_addr;

    if(connect(m_d->m_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
      return errno;

    return 0;
  }

  bool TCPSocket::close()
  {
    if(m_d->m_fd < 0)
      return false;

    ::close(m_d->m_fd);

    m_d->m_fd = -1;

    return true;
  }

  bool TCPSocket::isOpen() const
  {
    return (m_d->m_fd > 0);
  }

  int TCPSocket::read(void * buffer, int bytes, bool waitfordata)
  {
    if(m_d->m_fd < 0)
      return -1;

    int got = 0;
    char * ptr = (char *) buffer;

    while(got < bytes && (!waitfordata || isPendingInput(500000))) {
      int n = ::read(m_d->m_fd, ptr + got, bytes - got);
std::cout << "LL ";
      if(n < 0) {
    error("TCPSocket::read # n < 0");
    break;
      }

      got += n;

      if(!waitfordata) {
    return got;
      }
    }

    return got;
  }

  int TCPSocket::write(const void * buffer, int bytes)
  {
    if(m_d->m_fd < 0)
      return -1;

    return ::write(m_d->m_fd, buffer, bytes);
  }

  bool TCPSocket::isHungUp() const
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    bzero( & pfd, sizeof(pfd));
    pfd.fd = m_d->m_fd;
    pfd.events = ~0;
    poll(&pfd, 1, 0);

   return (pfd.revents & (POLLHUP)) != 0;
   // return (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
  }

  bool TCPSocket::isPendingInput(unsigned int waitMicroSeconds)
  {
    if(m_d->m_fd < 0)
      return false;

    struct pollfd pfd;
    bzero( & pfd, sizeof(pfd));

    pfd.fd = m_d->m_fd;
    pfd.events = POLLIN;
    poll(&pfd, 1, waitMicroSeconds / 1000);

    return pfd.revents & POLLIN;
  }

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

