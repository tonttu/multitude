#ifndef RADIANT_SOCKETUTILPOSIX_HPP
#define RADIANT_SOCKETUTILPOSIX_HPP

///@cond

#include <arpa/inet.h>
#include <netdb.h>


namespace Radiant {

  class SocketUtilPosix
  {
  public:

    /* Converts ascii text to in_addr struct.  NULL is returned if the address
       can not be found. */
    inline static struct in_addr * atoaddr(const char *address)

    {
      struct hostent *host;
      static struct in_addr saddr;

      /* First try it as aaa.bbb.ccc.ddd. */
      saddr.s_addr = inet_addr(address);
      if ((int) saddr.s_addr != -1) {
        return &saddr;
      }
      host = gethostbyname(address);
      if (host != 0) {
        return (struct in_addr *) *host->h_addr_list;
      }
      return 0;
    }

  };
}

///@endcond

#endif // SOCKETUTILPOSIX_HPP
