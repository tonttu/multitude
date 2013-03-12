/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TCP_SOCKET_HPP
#define RADIANT_TCP_SOCKET_HPP

#include <Patterns/NotCopyable.hpp>

#include <Radiant/BinaryStream.hpp>

#include <QString>

struct in_addr;

namespace Radiant {

  class Thread;

  /// A client TCP socket for connecting to remote hosts
  /** @author Tommi Ilmonen*/
  class RADIANT_API TCPSocket : public Radiant::BinaryStream, public Patterns::NotCopyable
  {
  public:
    /// Flags used with read() -function
    enum Flags
    {
      NONBLOCK,   ///< Never block
      WAIT_SOME,  ///< Block until some data is read or socket has a read error
      WAIT_ALL    ///< Blocks until all requested data is read or socket has a read error
    };

    TCPSocket();
    /// Construct a socket based on a file descriptor
    /// This method is potentially non-portable as not all platforms use file
    /// descriptors to handle sockets.
    /// @param fd socket file descriptor
    TCPSocket(int fd);
    ~TCPSocket();
    /// Move the given socket
    /// @param socket socket to move
    TCPSocket(TCPSocket && socket);
    /// Move the given socket
    /// @param socket socket to move
    /// @return reference to this
    TCPSocket & operator=(TCPSocket && socket);

    /// Turn the Nagle algorithm on or off. This controls the queuing of
    /// messages to fill packets.
    /// @param noDelay if true, the queing is not used and packets are sent immediately
    /// @return true on success
    bool setNoDelay(bool noDelay);

    /// Opens a TCP socket to desired host:port
    /// @param host hostname
    /// @param port port
    /// @return On successful execution, returns zero, otherwise an
    /// error code (as in errno.h).
    int open(const char * host, int port);
    /// Closes the socket
    virtual bool close();

    /// Returns true of the socket is open.
    bool isOpen() const;

    /// Returns the hostname
    //QString host() const;
    /// Returns the port number
    //int port() const;

    /// Read bytes from the socket
    /** @param[out] buffer pointer to a buffer to store the read data to
        @param bytes how many bytes the buffer has room for
        @param flags Blocking behaviour
        @return the number of bytes actually read
        */
    int read(void * buffer, int bytes, Flags flags);

    /// Read bytes from the socket
    /** @param[out] buffer pointer to a buffer to store the read data to
        @param bytes how many bytes the buffer has room for
        @param waitfordata wait for all data or do not block at all
        @see read(void*,int,Flags)
        @return the number of bytes actually read
        */
    virtual int read(void * buffer, int bytes, bool waitfordata = true)
    {
      return read(buffer, bytes, waitfordata ? WAIT_ALL : NONBLOCK);
    }

    /// Write bytes to the socket
    int write(const void * buffer, int bytes);

    /// Returns true if the socket has been closed
    virtual bool isHungUp() const;

    /// Return 'true' if readable data is pending.
    virtual bool isPendingInput(unsigned int waitMicroSeconds = 0);

    /// @cond
    int fd() const;
    /// @endcond

    /// Number of bytes received through the socket. This count gets cleared
    /// when a socket is opened.
    /// @return number of bytes received through the socket
    unsigned long rxBytes() const;
    /// Number of bytes sent through the socket. This count gets cleared
    /// when a socket is opened.
    /// @return number of bytes sent through the socket
    unsigned long txBytes() const;

/// @cond

    /// Currently not used.
    /// @param t destination thread
    void moveToThread(Thread * t);

/// @endcond

  private:
    friend class TCPServerSocket;

    class D;
    D * m_d;
  };

}

#endif
