/* COPYRIGHT
 */

#ifndef RADIANT_UDP_SOCKET_HPP
#define RADIANT_UDP_SOCKET_HPP

#include <Radiant/BinaryStream.hpp>

#include <QString>
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

    /** Opens a local server socket. This socket is generally good for listening to incoming
       messages.

       @param port The port number to listen to
       @return Zero on success, otherwise an error code
    */
    int openServer(int port);
    /** Opens a client socket for sending packets to given address.

        @param host The host address. On UNIX you can use both numeric (192.168.0.12),
        and human-readable (www.multitouch.fi) network names, while on Windows you
        can only use numeric names for the time being.

        @param port The port number to listen to
        @return Zero on success, otherwise an error code
    */
    int openClient(const char * host, int port);

    /// Returns true if the socket is open. Does not make much sense in the case of UDPSockets.
    bool isOpen() const;

    /// Closes the socket.
    bool close();

    /** Reads datagram packets from the socket.
        @param buffer buffer to write to
        @param bytes maximum bytes to read
        @param waitfordata blocks until there is something to read
        @return The number of bytes read is returned. If there was
        nothing to read, then zero is returned.*/
    virtual int read(void * buffer, int bytes, bool waitfordata);
    /** @copydoc read
        @param readAll if true, blocks until the buffer is full, otherwise
               return when we have some data*/
    virtual int read(void * buffer, int bytes, bool waitfordata, bool readAll);    

    /** Writes one datagram packet to the socket.
        @return The number of bytes written is returned.
    */
    virtual int write(const void *, int );

    /// Sets size of receive buffer
    bool setReceiveBufferSize(size_t bytes);

  private:
    class D;
    D * m_d;
  };

}

#endif
