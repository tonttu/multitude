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

#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/SerialPort.hpp>

#include <cassert>
#include <string>

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
    debug("SerialPort::open(%s)", device);
    close();
      
    m_device = device;

    // Open serial port

    const char * fName = "SerialPort::open";

    m_hPort = CreateFileA(device, GENERIC_READ | GENERIC_WRITE,
      0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    
    if(m_hPort == INVALID_HANDLE_VALUE)
    {
      const std::string   strErr = StringUtils::getLastErrorMessage();
      error("%s # Failed to open serial port (%s): %s", fName, device, strErr.c_str());

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
    assert(isOpen());

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

  int SerialPort::read(void * buf, int bytes)
  {
    assert(isOpen());

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

}
