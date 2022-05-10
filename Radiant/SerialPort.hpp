/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_SERIAL_PORT_HPP
#define RADIANT_SERIAL_PORT_HPP

#include <Radiant/BinaryStream.hpp>

#include <cstdint>

#ifdef WIN32
#include <memory>
#endif

#include <QStringList>

namespace Radiant
{

  /// A serial port handler.
  /** This class is for binary IO with serial ports.

      @author Tommi Ilmonen
  */

  // Doesn't inherit from BinaryStream because there doesn't seem to
  // be a decent way to do a non-blocking write on windows on anything
  // except sockets. You can do asynchronous writes, but they can't be
  // stopped if they would block. They happen in the background and write
  // all their data or error out.
  // However, non-blocking writes to serial ports are not currently used, so
  // we can delay this problem to the hopefully distant future when we might
  // actually need that functionality.
  class RADIANT_API SerialPort
  {
  public:
    /// Constructor
    SerialPort();
    /// Delete the object and close the port
    ~SerialPort();

    /// Move serialport
    SerialPort(SerialPort && port);

    /// Opens a serial port for communications
    /** If the port was open, this method will close it before opening it.
      @param device name of the device to open
      @param stopBit Use two stop bits instead of one
      @param parityBit Use parity stop bit?
      @param baud The baud rate
      @param bits The number of data bits
      @param waitBytes The number of bytes to read before returning
      @param waitTimeUS Time to wait in microseconds
      @return true on success
      */
    bool open(const char * device,
          bool stopBit,
          bool parityBit,
          int baud,
          int bits,
          int waitBytes,
          int waitTimeUS);

    /// Close the serial port.
    bool close();

    /// Performs a blocking write. Can call interrupt() to stop before
    /// timeout expires.
    /// @param timeoutSeconds zero or negative timeout means block until write is done
    /// @param ok pointer to boolean that holds error status. Will be set
    /// to false in case of an error. Timeouts and calling interrupt() are not
    /// errors. Can be null.
    /// @returns number of bytes written. Might be lower than requested
    /// due to timeout or interruption
    int write(const char *buf, int bytes,
              double timeoutSeconds = -1,
              bool *ok = nullptr);

    /// Performs a blocking write. Can call interrupt() to stop before
    /// timeout expires.
    /// @param timeoutSeconds zero or negative timeout means block until write is done
    /// @param ok pointer to boolean that holds error status. Will be set
    /// to false in case of an error. Timeouts and calling interrupt() are not
    /// errors. Can be null.
    /// @returns number of bytes written. Might be lower than requested
    /// due to timeout or interruption
    int write(const QByteArray &buffer,
              double timeoutSeconds = -1,
              bool *ok = nullptr);

    /// Performs a blocking read. Can call interrupt() to stop before
    /// the timeout expires.
    /// @param timeoutSeconds zero or negative timeout means block until there is data to read
    /// @param ok true if success, false on error
    /// @returns number of bytes read
    int read(char *buf, int bytes, double timeoutSeconds = -1, bool *ok = nullptr);

    /// Performs a blocking read. Can call interrupt() to stop before
    /// the timeout expires.
    /// If you need number of bytes read, then check output before and
    /// after the call.
    /// @returns false in case of an error, true otherwise. Timeouts or
    /// calling interrupt() are not errors.
    bool read(QByteArray &output, double timeoutSeconds = -1, int maxBytes = -1);

    /// Interrupts a blocking read before the timeout expires. On POSIX
    /// it might block while writing to a pipe (should be very short).
    void interruptRead();
    /// Interrupts a blocking write before the timeout expires. On POSIX it
    /// might block while writing to a pipe (should be very short).
    void interruptWrite();

    /// Checks if the port is open
    /// @return True if the port is open, false otherwise
    bool isOpen() const;

    /// Returns the name of the device
    /// @return Name of the device
    const QString & deviceName() { return m_device; }

    /// If name is not null, will print all read and written data
    void setTraceName(const char *name);

#ifdef RADIANT_UNIX
    /// @returns file descriptor
    int fd() const { return m_fd; }
#endif

#ifdef RADIANT_WINDOWS
    /// @todo implement this on all platforms
    /// @returns list of all found serial ports on the system
    static QStringList scan();
#endif

    enum class WaitStatus { Ok, Error, Interrupt };

  private:
    SerialPort(const SerialPort & port);

  private:
    QString m_device;
    const char *m_traceName;

#ifdef WIN32
    struct D;
    std::unique_ptr<D> m_d;
#else
    int m_fd;
    // Will poll on both the serial port and one of the interrupt pipe ends.
    // if an interrupt is desired, we can write to the other end of the pipe
    // to break out of the poll call.
    int m_readInterruptPipe[2];
    int m_writeInterruptPipe[2];


    WaitStatus wait(int events, double timeoutSeconds, int pipe);

    int doRead(void *buf, int bytes);
    int doWrite(const void * buf, int bytes);

    enum class WriteStatus {
      Ok,
      WouldBlock,
      WriteError,
    };
    int doWrite(const char *buf, int bytes, WriteStatus *status);

    void interrupt(int pipe);
#endif
  };
}

#endif


