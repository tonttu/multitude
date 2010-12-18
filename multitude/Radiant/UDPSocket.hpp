/* COPYRIGHT
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

    // int open(const char * host, int port, bool client);
    int openServer(int port);
    int openClient(const char * host, int port);

    /// Returns true if the socket is open. Does not make much sense in the case of UDPSockets.
    bool isOpen() const;

    bool close();

    /** Reads one datagram packet from the socket.

        @return The number of bytes read is returned. If there was
        nothing to read, then zero is returned.

        If there are multiple datagrams to be read, you need to use
        this function multiple times, even if the buffer was large
        enough to contain multiple packets.
    */
    // int readDatagram(char * data, size_t maxSize, std::string * fromAddr, uint16_t * fromPort = 0);

    /** Writes one datagram packet to the socket.

        @return The number of bytes written is returned.

    */
    // int writeDatagram(const char * data, size_t bytes, const std::string & host, uint16_t port);

    virtual int read(void *, int , bool );
    virtual int write(const void *, int );

  private:
    class D;
    D * m_d;
  };

}

#endif
