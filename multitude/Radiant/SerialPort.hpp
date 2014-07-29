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
# define NOMINMAX
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include <QStringList>

namespace Radiant
{

  /// A serial port handler.
  /** This class is for binary IO with serial ports.

      @author Tommi Ilmonen
  */
  class RADIANT_API SerialPort : public Radiant::BinaryStream
  {
  public:
    /// Constructor
    SerialPort();
    /// Delete the object and close the port
    virtual ~SerialPort();

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

    /// Write bytes to the port. Does not block.
    /// @param buf buffer to write from
    /// @param bytes bytes to write
    /// @return number of bytes written
    virtual int write(const void * buf, int bytes);

    /// Performs a blocking write. Can call interrupt() to stop before
    /// timeout expires.
    /// @param ok pointer to boolean that holds error status. Will be set
    /// to false in case of an error. Timeouts and calling interrupt() are not
    /// errors. Can be null.
    /// @returns number of bytes written. Might be lower than requested
    /// due to timeout or interruption
    int blockingWrite(const QByteArray &buffer,
                      double timeoutSeconds,
                      bool *ok = nullptr);

    /// Writes a byte to the port
    /// @param byte Byte to write
    /// @return Number of bytes written
    int writeByte(uint8_t byte);

    /// Read bytes from the port. Does not block.
    /// @param[out] buf output buffer
    /// @param bytes max bytes to read
    /// @param waitfordata ignored
    /// @return number of bytes read
    virtual int read(void *buf, int bytes, bool waitfordata = true);

    /// Performs a blocking read. Can call interrupt() to stop before
    /// the timeout expires.
    /// If you need number of bytes read, then check output before and
    /// after the call.
    /// @returns false in case of an error, true otherwise. Timeouts or
    /// calling interrupt() are not errors.
    bool blockingRead(QByteArray *output, double timeoutSeconds);

    /// Interrupts a blocking read or write before the timeout expires.
    void interrupt();

    /// Checks if the port is open
    /// @return True if the port is open, false otherwise
    bool isOpen() const;

    /// Returns the name of the device
    /// @return Name of the device
    const QString & deviceName() { return m_device; }

#ifdef RADIANT_UNIX
    /// @returns file descriptor
    int fd() const { return m_fd; }
#endif

#ifdef RADIANT_WINDOWS
    /// @todo implement this on all platforms
    /// @returns list of all found serial ports on the system
    static QStringList scan();
#endif

  private:
    QString m_device;

#ifdef WIN32
    HANDLE  m_hPort;
#else
    int m_fd;
    // Will poll on both the serial port and one of the interrupt pipe ends.
    // if an interrupt is desired, we can write to the other end of the pipe
    // to break out of the poll call.
    int m_interruptPipe[2];
    bool wait(int events, double timeoutSeconds);
#endif

  };

}

#endif


