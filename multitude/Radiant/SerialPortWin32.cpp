/* COPYRIGHT
 */

#include "Platform.hpp"

#ifdef RADIANT_WINDOWS

#include "Radiant.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <QSettings>

namespace Radiant
{

  SerialPort::SerialPort()
  : m_hPort(0)
  {}

  SerialPort::~SerialPort()
  {
    close();
  }

  bool SerialPort::open(const char * device, bool /*stopBit*/, bool parityBit,
    int baud, int bits, int /*waitBytes*/, int /*waitTimeUS*/)
  {
    // First make sure serial port is closed
    debugRadiant("SerialPort::open(%s)", device);
    close();

    // Make the devicename compliant to new addressing (needed for >COM9)
    m_device = QString("\\\\.\\") + device;

    // Open serial port

    const char * fName = "SerialPort::open";

    m_hPort = CreateFileA(m_device.toUtf8().data(), GENERIC_READ | GENERIC_WRITE,
      0, 0, OPEN_EXISTING, 0, 0);

    if(m_hPort == INVALID_HANDLE_VALUE)
    {
      const QString strErr = StringUtils::getLastErrorMessage();
      error("%s # Failed to open serial port (%s): %s", fName, device, strErr.toUtf8().data());

      m_hPort = 0;
      return false;
    }

    // Obtain current parameters of serial port

    DCB dcbParams;
    memset(& dcbParams, 0, sizeof(dcbParams));
    dcbParams.DCBlength = sizeof(dcbParams);

    if(!GetCommState(m_hPort, & dcbParams))
    {
      error("%s # Failed to get serial port state (%s)", fName, device);
      close();
      return false;
    }

    // Set parameters for serial port

    int   cbr = 0;
    switch(baud)
    {
      case 110:     cbr = CBR_110;    break;
      case 300:     cbr = CBR_300;    break;
      case 600:     cbr = CBR_600;    break;
      case 1200:    cbr = CBR_1200;   break;
      case 2400:    cbr = CBR_2400;   break;
      case 4800:    cbr = CBR_4800;   break;
      case 9600:    cbr = CBR_9600;   break;
      case 14400:   cbr = CBR_14400;  break;
      case 19200:   cbr = CBR_19200;  break;
      case 38400:   cbr = CBR_38400;  break;
      case 56000:   cbr = CBR_56000;  break;
      case 57600:   cbr = CBR_57600;  break;
      case 115200:  cbr = CBR_115200; break;
      case 128000:  cbr = CBR_128000; break;
      case 256000:  cbr = CBR_256000; break;
      default:
      {
        error("%s # Invalid baud rate (%d)", fName, baud);
        close();
        return false;
      }
    }
    dcbParams.BaudRate = cbr;
    dcbParams.ByteSize = BYTE(bits);
    dcbParams.StopBits = ONESTOPBIT;
    dcbParams.Parity = parityBit ? EVENPARITY : NOPARITY;

    if(!SetCommState(m_hPort, & dcbParams))
    {
      error("%s # Failed to set serial port state (%s)", fName, device);
      close();
      return false;
    }

    // Set timeouts
//    const int   waitTimeMS = waitTimeUS / 1000;

//trace(INFO, "SerialPort::open # timeout %dus (%dms)", waitTimeUS, waitTimeMS);

    COMMTIMEOUTS  timeouts;
    memset(& timeouts, 0, sizeof(COMMTIMEOUTS));
    // Returns immediately
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    if(!SetCommTimeouts(m_hPort, & timeouts))
    {
      error("%s # Failed to set serial port timeouts (%s)", fName, device);
      close();
      return false;
    }

    return true;
  }

  bool SerialPort::close()
  {
    if(!isOpen())
    {
      return true;
    }

    bool  closed = false;
    if(CloseHandle(m_hPort))
    {
      closed = true;
      m_hPort = 0;
    }

    return closed;
  }

  int SerialPort::write(const void * buf, int bytes)
  {
    if(!isOpen()) {
      error("SerialPort::write # device not open");
      return -1;
    }

    DWORD   bytesWritten = 0;
    WriteFile(m_hPort, buf, bytes, & bytesWritten, 0);
/*
    std::ostringstream os;
    for(int i = 0; i < bytesWritten; i++)
        os << (int)((char*)buf)[i] << " ";

    info("SerialPort::write # wrote %d bytes (%s)", bytesWritten, os.str().c_str());
*/
    return int(bytesWritten);
  }

  int SerialPort::writeByte(uint8_t byte)
  {
    return write(& byte, 1);
  }

  int SerialPort::read(void * buf, int bytes, bool waitfordata)
  {
    if(!isOpen()) {
      error("SerialPort::read # device not open");
      return -1;
    }

    if(!waitfordata) {
      error("SerialPort::read # !waitfordata nor supported yet.");
    }

    DWORD   bytesRead = 0;
    if(ReadFile(m_hPort, buf, bytes, & bytesRead, 0) == FALSE)
        error("SerialPort::read # read failed");
/*

    std::ostringstream os;
    for(int i = 0; i < bytesRead; i++)
        os << (int)((char*)buf)[i] << " ";
    info("SerialPort::read # read %d bytes (%s)", bytesRead, os.str().c_str());
*/

    return int(bytesRead);
  }

  bool SerialPort::isOpen() const
  {
    return (m_hPort != 0);
  }

  QStringList SerialPort::scan()
  {
    QStringList ports;
    HKEY key;
    /// QSettings doesn't support registry values that have '\' or '/' in
    /// their names, so we are forced to use raw win api.
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM",
                      0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
      Radiant::error("SerialPort::scan # Failed to open \"HARDWARE\\DEVICEMAP\\SERIALCOMM\"");
      return ports;
    }

    for (int i = 0; ; ++i) {
      QByteArray name(255, '\0');
      QByteArray value(255, '\0');
      DWORD nameSize = name.size();
      DWORD valueSize = value.size();
      long err = RegEnumValueA(key, i, name.data(), &nameSize, nullptr, nullptr, (LPBYTE)value.data(), &valueSize);
      if (err == ERROR_NO_MORE_ITEMS)
        break;
      if (err == ERROR_SUCCESS) {
        if (name.startsWith("\\Device\\"))
          ports << value;
      } else {
        Radiant::error("SerialPort::scan # RegEnumValueA error %d", err);
      }
    }
    RegCloseKey(key);

    return ports;
  }

}

#endif
