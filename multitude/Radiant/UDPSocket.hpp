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

#ifndef RADIANT_UDP_SOCKET_HPP
#define RADIANT_UDP_SOCKET_HPP

#include <Radiant/BinaryStream.hpp>

#include <string>
#include <stdint.h>


namespace Radiant
{
  
  /** UPD socket implementation.
      
      UDP is an unreliable socket type, where data is move in datagram
      packages.

      Packages have some limited maximum size which depends on the
      network, and can only be deduced dynamically in the
      run-time. Usually the maximum packet sizes are in the range
      4-8kb.

      @see TCPSocket
  */
  class RADIANT_API UDPSocket : public Radiant::BinaryStream
  {
  public:
    UDPSocket();

    /** Constructs a new UDP socket and initializes it to the given file
    descriptor*/
    UDPSocket(int fd);
    ~UDPSocket();

    /** Binds this socket to the given address and port. This is useful when you want to read from a UDP socket. */
    bool bind(const std::string & address, uint16_t port);

    /// Returns true if the socket is open. Does not make much sense in the case of UDPSockets.
    bool isOpen() const;
    
    /** Reads one datagram packet from the socket. 

        @return The number of bytes read is returned. If there was
        nothing to read, then zero is returned.
        
        If there are multiple datagrams to be read, you need to use
        this function multiple times, even if the buffer was large
        enough to contain multiple packets.
    */
    int readDatagram(char * data, size_t maxSize, std::string * fromAddr, uint16_t * fromPort = 0);

    /** Writes one datagram packet to the socket. 

        @return The number of bytes written is returned.
    
    */
    int writeDatagram(const char * data, size_t bytes, const std::string & host, uint16_t port);

    /// Not implemented for UDPSocket
    virtual int read(void *, int , bool ) { return -1; }
    /// Not implemented for UDPSocket
    virtual int write(const void *, int ) { return -1; }
    
  private:
    class D;
    D * m_d;
  };

}

#endif
