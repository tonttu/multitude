/* COPYRIGHT
 */

#ifndef RADIANT_SERIAL_PORT_HPP
#define RADIANT_SERIAL_PORT_HPP

#include <Radiant/BinaryStream.hpp>

#include <stdint.h>

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

    /// Write bytes to the port
    /// @param buf buffer to write from
    /// @param bytes bytes to write
    /// @return number of bytes written
    virtual int write(const void * buf, int bytes);
    /// Writes a byte to the port
    int writeByte(uint8_t byte);
    /// Read bytes from the port
    /// @param[out] buf output buffer
    /// @param bytes max bytes to read
    /// @return number of bytes read
    virtual int read(void * buf, int bytes, bool waitfordata = true);

    /// Checks if the port is open
    bool isOpen() const;
    /// Returns the name of the device
    const QString & deviceName() { return m_device; }

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
    int   m_fd;
#endif

  };

}

#endif


