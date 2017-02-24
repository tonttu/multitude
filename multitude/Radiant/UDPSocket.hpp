/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_UDP_SOCKET_HPP
#define RADIANT_UDP_SOCKET_HPP

#include <Radiant/BinaryStream.hpp>
#include <Radiant/TimeStamp.hpp>

#include <QString>
#include <cstdint>


namespace Radiant
{

  /** UPD socket implementation.

      UDP is an unreliable socket type, where data is move in datagram
      packages.

      Packages have some limited maximum size which depends on the
      network, and can only be deduced dynamically in the
      run-time. Usually the maximum packet sizes are in the range
      4-8kb.

      @sa TCPSocket
  */
  class RADIANT_API UDPSocket : public Radiant::BinaryStream
  {
  public:
    /// Constructor
    UDPSocket();

    /// Constructs a new UDP socket and initializes it to the given file
    /// descriptor.
    /// @param fd File descriptor for the socket
    UDPSocket(int fd);
    /// Destructor
    ~UDPSocket();

    /** Opens a local server socket. This socket is generally good for listening to incoming
       messages.

       @param port The port number to listen to
       @param bindAddress Address that the socket should be bound to
       @return Zero on success, otherwise an error code
    */
    int openServer(int port, const char * bindAddress = "0.0.0.0");
    /** Opens a client socket for sending packets to given address.

        @param host The host address. On UNIX you can use both numeric (192.168.0.12),
        and human-readable (www.multitouch.fi) network names, while on Windows you
        can only use numeric names for the time being.

        @param port The port number to listen to
        @return Zero on success, otherwise an error code
    */
    int openClient(const char * host, int port);

    /// Returns true if the socket is open. Does not make much sense in the case of UDPSockets.
    /// @return True if the socket is open.
    bool isOpen() const;

    /// Closes the socket.
    /// @return True if the socket was succesfully closed.
    bool close();

    virtual bool isPendingInput(unsigned int waitMicroSeconds);

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
        @param data Data to write
        @param bytes How many bytes are requested to write
        @return The number of bytes written is returned.
    */
    virtual int write(const void *data, int bytes);

    /// Sets size of receive buffer
    /// @param bytes Requested number of bytes for the receiving buffer
    /// @return True if the size of the receive buffer was succusfully updated
    bool setReceiveBufferSize(size_t bytes);

#ifdef RADIANT_LINUX
    /// @returns timestamp of the last packet read
    Radiant::TimeStamp timestamp() const;

    /// @return file descriptor of the socket, or -1 if invalid
    int fd() const;
#endif

  private:
    class D;
    D * m_d;
  };

}

#endif
