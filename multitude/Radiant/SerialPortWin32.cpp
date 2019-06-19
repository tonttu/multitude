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

  struct SerialPort::D {
    SerialPort &m_host;
    HANDLE m_hPort;
    OVERLAPPED m_overlappedRead;
    OVERLAPPED m_overlappedWrite;

    void interrupt(OVERLAPPED *overlapped);
    WaitStatus waitUntilCanRead(double timeoutSeconds);
    int doRead(char *buf, int maxSize, bool *ok);

    D(SerialPort& host) : m_host(host), m_hPort(0) {
      m_overlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      m_overlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
  };

  SerialPort::SerialPort()
  : m_traceName(nullptr),
    m_d(new D(*this))
  {}

  SerialPort::SerialPort(SerialPort && port)
    : m_device(port.m_device)
  {
    m_d->m_hPort = port.m_d->m_hPort;

    port.m_device.clear();
    port.m_d->m_hPort = 0;
  }

  SerialPort::~SerialPort()
  {
    close();
  }

  void SerialPort::setTraceName(const char *name)
  {
    m_traceName = name;
  }

  static void clearOverlapped(OVERLAPPED &overlapped)
  {
    overlapped.Internal = 0;
    overlapped.InternalHigh = 0;
    overlapped.Pointer = 0;
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

    bool closed = false;
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
    bool pending = ret == 0 && GetLastError() == ERROR_IO_PENDING;
    bool completedSynchronously = ret != 0;
    if(ret == 0 && !pending && GetLastError() != ERROR_OPERATION_ABORTED) {
      printLastError("Write - WriteFile");
      safeset(ok, false);
      return 0;
    }

    DWORD bytesWritten = 0;
    if(pending || completedSynchronously) {
      // make sure we don't call GetOverlappedResult when aborted, else it will hang
      ret = GetOverlappedResult(m_d->m_hPort, &m_d->m_overlappedWrite, &bytesWritten, TRUE);
      if(ret == 0
         && GetLastError() != ERROR_OPERATION_ABORTED
         && GetLastError() != ERROR_IO_INCOMPLETE) {
        printLastError("Write - GetOverlappedResult");
        safeset(ok, false);
        return 0;
      }
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

  SerialPort::WaitStatus SerialPort::D::waitUntilCanRead(double timeoutSeconds)
  {
    if(m_hPort == 0) {
      error("SerialPort::read # device not open");
      return WaitStatus::Error;
    }

    // wait only for RXCHAR events
    BOOL ok = SetCommMask(m_hPort, EV_RXCHAR);
    if(!ok) {
      printLastError("Read - SetCommMask");
      return WaitStatus::Error;
    }

    DWORD event = -1;
    OVERLAPPED& overlapped = m_overlappedRead;
    clearOverlapped(overlapped);
    ok = WaitCommEvent(m_hPort, &event, &overlapped);
    bool pending = !ok && GetLastError() == ERROR_IO_PENDING;
    if(!ok && !pending) {
      printLastError("Read - WaitCommEvent");
      return WaitStatus::Error;
    }

    if(pending) {
      DWORD res = WaitForSingleObject(overlapped.hEvent, timeoutSeconds * 1000);
      switch(res) {
      case WAIT_OBJECT_0:
        // got notified of some available bytes.
        return WaitStatus::Ok;
      case WAIT_TIMEOUT:
        // timeout, skip reading, return 0 bytes read
        return WaitStatus::Interrupt;
      default:
        // An error occured
        printLastError("Read - WaitForSingleObject");
        return WaitStatus::Error;
      }
    }

    return WaitStatus::Ok;
  }

  int SerialPort::D::doRead(char *buffer, int maxRead, bool *readOk)
  {
    // set magic timeouts that mean - give me available data immediately and
    // do not block even when I ask for more data than you have.
    COMMTIMEOUTS timeouts;
    ZeroMemory(&timeouts, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = MAXDWORD;
    BOOL ok = SetCommTimeouts(m_hPort, &timeouts);
    if(!ok) {
      printLastError("Read - SetCommTimeouts");
      safeset(readOk, false);
      return 0;
    }

    // can now read and it should not block due to magic timeouts
    OVERLAPPED& overlapped = m_overlappedRead;
    clearOverlapped(overlapped);
    DWORD ret = ReadFile(m_hPort, buffer, maxRead, NULL, &m_overlappedRead);
    bool pending = ret == 0 && GetLastError() == ERROR_IO_PENDING;
    bool completedSynchronously = ret != 0;
    if(ret == 0 && !pending && GetLastError() != ERROR_OPERATION_ABORTED) {
      printLastError("Read - ReadFile");
      safeset(readOk, false);
      return 0;
    }
    if(pending) {
      Radiant::error("Win32 SerialPort - Pending ReadFile in spite of magic timeouts");
    }

    DWORD bytesRead = 0;
    if(pending || completedSynchronously) {
      // make sure we don't call GetOverlappedResult if the operation was aborted.
      // this should not block.
      ret = GetOverlappedResult(m_hPort, &m_overlappedRead, &bytesRead, TRUE);
      if(ret == 0
         && GetLastError() != ERROR_OPERATION_ABORTED
         && GetLastError() != ERROR_IO_INCOMPLETE) {
        printLastError("Read - GetOverlappedResult");
        safeset(readOk, false);
        return 0;
      }
    }

    if(bytesRead > 0 && m_host.m_traceName != nullptr) {
      printBuffer(buffer, (int)bytesRead, ">", m_host.m_traceName);
    }

    return bytesRead;
  }

  int SerialPort::read(char *buffer, int maxRead, double timeoutSeconds, bool *readOk)
  {
    safeset(readOk, true);

    if(!isOpen()) {
      error("SerialPort::read # device not open");
      safeset(readOk, false);
      return 0;
    }

    WaitStatus status = m_d->waitUntilCanRead(timeoutSeconds);
    switch(status) {
    case WaitStatus::Ok:
      break;
    case WaitStatus::Interrupt:
      return 0;  // time's up, did not read anything
    default:
      // an error occured, return without reading
      safeset(readOk, false);
      return 0;
    }

    return m_d->doRead(buffer, maxRead, readOk);
  }

  bool SerialPort::read(QByteArray &output, double timeoutSeconds, int maxBytes)
  {
    if(!isOpen()) {
      error("SerialPort::read # device not open");
      return false;
    }

    WaitStatus status = m_d->waitUntilCanRead(timeoutSeconds);
    switch(status) {
    case WaitStatus::Ok:
      break;
    case WaitStatus::Interrupt:
      return true;  // time's up, did not read anything
    default:
      // an error occured, return without reading
      return false;
    }

    static const int bufSize = 1024;
    int bytes = -1;
    int startSize = output.size();
    do {
      int oldSize = output.size();
      output.resize(output.size() + bufSize);
      char *buffer = output.data() + oldSize;
      bool ok;
      int maxThisRead = bufSize;
      if(maxBytes > 0) {
        int soFar = output.size() - startSize;
        maxThisRead = std::min(maxBytes - soFar, bufSize);
      }
      bytes = m_d->doRead(buffer, maxThisRead, &ok);
      if(ok) {
        output.resize(oldSize + bytes);
      } else {
        output.resize(oldSize);
        return false;
      }
    } while(bytes > 0);
    return true;
  }

  bool SerialPort::isOpen() const
  {
    return (m_d->m_hPort != 0);
  }

  void SerialPort::D::interrupt(OVERLAPPED *overlapped)
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
