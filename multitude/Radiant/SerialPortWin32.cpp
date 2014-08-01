/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#include "Platform.hpp"

#ifdef RADIANT_WINDOWS

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Radiant.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <QSettings>

#include "SerialPortHelpers.hpp"

namespace Radiant
{
  static void printLastError(const char *context)
  {
    DWORD errorCode = GetLastError();
    LPVOID msgBuf;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
                   | FORMAT_MESSAGE_ALLOCATE_BUFFER
                   | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   errorCode,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (LPSTR)&msgBuf,
                   0,
                   NULL);
    Radiant::error("SerialPort Win32 error in %s: %s", context, msgBuf);
    LocalFree(msgBuf);
  }

  struct SerialPort::Impl {
    HANDLE m_hPort;
    OVERLAPPED m_overlappedRead;
    OVERLAPPED m_overlappedWrite;

    void interrupt(OVERLAPPED *overlapped);
    Impl() : m_hPort(0) { }
  };

  SerialPort::SerialPort()
  : m_d(new Impl()),
    m_traceName(nullptr)
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

    // Careful, OVERLAPPED flag forces us to use async reads and writes all the time.
    // This prevents the use of eager read / writes which return when they would block,
    // like you can use on linux with O_NONBLOCK
    // The windows async apis are all or nothing. You can cancel, but that requires
    // blocking first.
    m_d->m_hPort = CreateFileA(m_device.toUtf8().data(), GENERIC_READ | GENERIC_WRITE,
      0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);

    if(m_d->m_hPort == INVALID_HANDLE_VALUE)
    {
      const QString strErr = StringUtils::getLastErrorMessage();
      error("%s # Failed to open serial port (%s): %s", fName, device, strErr.toUtf8().data());

      m_d->m_hPort = 0;
      return false;
    }

    // Obtain current parameters of serial port

    DCB dcbParams;
    memset(& dcbParams, 0, sizeof(dcbParams));
    dcbParams.DCBlength = sizeof(dcbParams);

    if(!GetCommState(m_d->m_hPort, & dcbParams))
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

    if(!SetCommState(m_d->m_hPort, & dcbParams))
    {
      error("%s # Failed to set serial port state (%s)", fName, device);
      close();
      return false;
    }

    // Set timeouts to zero initially (block forever). Will update before read / write ops
    // with correct values
    COMMTIMEOUTS  timeouts;
    memset(& timeouts, 0, sizeof(COMMTIMEOUTS));

    if(!SetCommTimeouts(m_d->m_hPort, & timeouts))
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
    if(CloseHandle(m_d->m_hPort))
    {
      closed = true;
      m_d->m_hPort = 0;
    }

    return closed;
  }

  int SerialPort::write(const char *buf, int bytes, double timeoutSeconds, bool *ok)
  {
    safeset(ok, true);

    if(!isOpen()) {
      error("SerialPort::write # device not open");
      safeset(ok, false);
      return 0;
    }

    if(timeoutSeconds < 0) {
      timeoutSeconds = 0;
    }

    COMMTIMEOUTS timeouts;
    ZeroMemory(&timeouts, sizeof(timeouts));
    timeouts.WriteTotalTimeoutConstant = timeoutSeconds * 1000;
    BOOL ret = SetCommTimeouts(m_d->m_hPort, &timeouts);
    if(ret == 0) {
      printLastError("Write - SetCommTimeouts");
      safeset(ok, false);
      return 0;
    }

    ZeroMemory(&m_d->m_overlappedWrite, sizeof(m_d->m_overlappedWrite));
    ret = WriteFile(m_d->m_hPort, buf, bytes, NULL, &m_d->m_overlappedWrite);
    if(ret == 0
       && GetLastError() != ERROR_IO_PENDING
       && GetLastError() != ERROR_OPERATION_ABORTED) {
      printLastError("Write - WriteFile");
      safeset(ok, false);
      return 0;
    }

    DWORD bytesWritten = 0;
    ret = GetOverlappedResult(m_d->m_hPort, &m_d->m_overlappedWrite, &bytesWritten, TRUE);
    if(ret == 0) {
      printLastError("Write - GetOverlappedResult");
      safeset(ok, false);
      return 0;
    }

    if(bytesWritten > 0 && m_traceName != nullptr) {
      printBuffer(buf, (int)bytesWritten, "<", m_traceName);
    }

    return int(bytesWritten);
  }

  int SerialPort::write(const QByteArray &buffer, double timeoutSeconds, bool *ok)
  {
    return write(buffer.data(), buffer.size(), timeoutSeconds, ok);
  }

  int SerialPort::read(char *buffer, int maxRead, double timeoutSeconds, bool *ok)
  {
    safeset(ok, true);

    if(!isOpen()) {
      error("SerialPort::read # device not open");
      safeset(ok, false);
      return 0;
    }

    COMMTIMEOUTS timeouts;
    ZeroMemory(&timeouts, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant = timeoutSeconds * 1000;
    BOOL ret = SetCommTimeouts(m_d->m_hPort, &timeouts);
    if(ret == 0) {
      printLastError("Read - SetCommTimeouts");
      safeset(ok, false);
      return 0;
    }

    ZeroMemory(&m_d->m_overlappedRead, sizeof(m_d->m_overlappedRead));
    ret = ReadFile(m_d->m_hPort, buffer, maxRead, NULL, &m_d->m_overlappedRead);
    if(ret == 0
       && GetLastError() != ERROR_IO_PENDING
       && GetLastError() != ERROR_OPERATION_ABORTED) {
      printLastError("Read - ReadFile");
      safeset(ok, false);
      return 0;
    }

    DWORD bytesRead = 0;
    ret = GetOverlappedResult(m_d->m_hPort, &m_d->m_overlappedRead, &bytesRead, TRUE);
    if(ret == 0) {
      printLastError("Write - GetOverlappedResult");
      safeset(ok, false);
      return 0;
    }

    if(bytesRead > 0 && m_traceName != nullptr) {
      printBuffer(buffer, (int)bytesRead, ">", m_traceName);
    }

    return bytesRead;
  }

  bool SerialPort::read(QByteArray *output, double timeoutSeconds)
  {
    // will read at most 4k of data. It's a stupid limitation but
    // this API is horrible. There's no easy way to iterate through the
    // available data while blocking only once if there is more data
    // than we have space for it. If it reports reading exactly maxRead
    // bytes it might mean that there is more data, or it might block.
    // It would work in non-overlapped mode with some magic values for the
    // timeouts, but that makes canceling only reads or only writes impossible.
    int maxRead = 4 * 1024;
    int oldSize = output->size();
    output->resize(output->size() + maxRead);
    char *buffer = output->data() + oldSize;
    bool ok;
    int bytes = read(buffer, maxRead, timeoutSeconds, &ok);
    if(ok) {
      output->resize(oldSize + bytes);
    } else {
      output->resize(oldSize);
    }
    return ok;
  }

  bool SerialPort::isOpen() const
  {
    return (m_d->m_hPort != 0);
  }

  void SerialPort::Impl::interrupt(OVERLAPPED *overlapped)
  {
    if(m_hPort == 0) {
      return;
    }
    BOOL ok = CancelIoEx(m_hPort, overlapped);
    if(ok == 0) {
      printLastError("InterruptRead - CancelIoEx");
    }
  }

  void SerialPort::interruptRead()
  {
    m_d->interrupt(&m_d->m_overlappedRead);
  }

  void SerialPort::interruptWrite()
  {
    m_d->interrupt(&m_d->m_overlappedWrite);
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
