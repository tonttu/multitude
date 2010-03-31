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
    UDPSocket(int fd);
    ~UDPSocket();
    
    /** Opens a UDP socket in either client or server mode. 

        @return On success, zero is returned. On failure an error code
        is returned.
    */
    int open(const char * host, int port, bool client = true);
    /** Opens a UDP socket in server mode.
    */

    int openServer(const char * host, int port);
    /** Opens a UDP socket in client mode.
    */
    int openClient(const char * host, int port);
    /** Closes the socket. */
    bool close();
    
    bool isOpen() const;
    
    /** Reads one datagram packet from the socket. 

        @return The number of bytes read is returned. If there was
        nothing to read, then zero is returned.
        
        If there are multiple datagrams to be read, you need to use
        this function multiple times, even if the buffer was large
        enough to contain multiple packets.
    */
    int read(void * buffer, int bytes, bool waitfordata = true);

    /** Writes one datagram packet to the socket. 

        @return The number of bytes written is returned.
    
    */
    int write(const void * buffer, int bytes);
    
  private:
    class D;
    D * m_d;
  };

}

#endif
