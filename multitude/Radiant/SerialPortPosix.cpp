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

#ifdef RADIANT_UNIX

#include "SerialPort.hpp"
#include "Trace.hpp"
#include "Timer.hpp"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <assert.h>

namespace Radiant
{

  SerialPort::SerialPort()
  : m_traceName(nullptr),
    m_fd(-1),
    m_readInterruptPipe{-1, -1},
    m_writeInterruptPipe{-1, -1}
  {}

  SerialPort::~SerialPort()
  {
    close();
  }

  bool SerialPort::open(const char * device, bool stopBit, bool parityBit,
            int baud, int bits, int waitBytes, int waitTimeUS)
  {
    close();

    m_device = device;

    const char * fname = "SerialPort::open";

    m_fd = ::open(device, O_RDWR | O_NOCTTY | O_NDELAY);

    if(m_fd <= 0) {
      Radiant::error("%s # Failed to open \"%s\" (%s)",
             fname, device, strerror(errno));
      errno = 0;
      return false;
    }

    struct termios opts;
    if (tcgetattr(m_fd, & opts) < 0) {
      Radiant::error("%s # Could get read port attributes (%s)", fname, device);
      close();
      return false;
    }

    if(stopBit)
      opts.c_cflag &= ~CSTOPB;
    else
      opts.c_cflag |= CSTOPB;

    if(parityBit)
      opts.c_cflag |= PARENB;
    else
      opts.c_cflag &= ~PARENB;

    opts.c_cflag &= ~CSIZE;

    if(bits == 5)
      opts.c_cflag |= CS5;
    else if(bits == 6)
      opts.c_cflag |= CS6;
    else if(bits == 7)
      opts.c_cflag |= CS7;
    else if(bits == 8)
      opts.c_cflag |= CS8;

    // Clear bauds:
    speed_t speed = baud;

    // Set bauds:
    if(baud == 1200)
      speed = B1200;
    else if(baud == 2400)
      speed =  B2400;
    else if(baud == 4800)
      speed =  B4800;
    else if(baud == 9600)
      speed =  B9600;
    else if(baud == 19200)
      speed =  B19200;
    else if(baud == 38400)
      speed =  B38400;
    else if(baud == 57600)
      speed =  B57600;
    else if(baud == 115200)
      speed =  B115200;
#ifdef B230400
    else if(baud == 230400)
      speed =  B230400;
#endif

    cfsetispeed( & opts, speed);
    cfsetospeed( & opts, speed);

    // Disable flow control
    opts.c_cflag &= ~CRTSCTS;

    opts.c_cflag |= CREAD;	/* Allow reading */
    opts.c_cflag |= CLOCAL;	/* No modem between us and device */

    opts.c_cc[VMIN] = waitBytes;
    opts.c_cc[VTIME] = waitTimeUS / 100000; /* Unit = 0.1 s*/


    opts.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    opts.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    opts.c_oflag &= ~OPOST; // make raw

    // disable CR-NL translation/mapping
    opts.c_iflag &= ~(INLCR | ICRNL);
    opts.c_oflag &= ~(OCRNL | ONLCR);

    if(tcsetattr(m_fd, TCSANOW, & opts) < 0) {
      Radiant::error("%s # Failed to set TTY parameters (%s)", fname, strerror(errno));
      close();
      errno = 0;
      return false;
    }

    if(pipe2(m_readInterruptPipe, O_NONBLOCK) != 0) {
      Radiant::error("Failed to create read interrupt pipe: %s", strerror(errno));
      return false;
    }

    if(pipe2(m_writeInterruptPipe, O_NONBLOCK) != 0) {
      Radiant::error("Failed to create write interrupt pipe: %s", strerror(errno));
      return false;
    }

    return true;
  }

  void SerialPort::setTraceName(const char *name)
  {
    m_traceName = name;
  }

  static void printBuffer(const char *buffer, int len, const char *op, const char *traceName)
  {
    const char *begin = buffer;
    const char *end = buffer + len;
    while(begin < end) {
      const char* zeroPtr = (const char*)memchr(begin, 0, end - begin);
      if(zeroPtr == nullptr) {
        zeroPtr = end;
      }
      // print chars before null
      int lenToPrint = zeroPtr - begin;
      if(lenToPrint > 0) {
        Radiant::info("%s%s: %.*s", traceName, op, lenToPrint, begin);
      }
      begin = begin + lenToPrint;
      // now consume all nulls
      while(*begin == '\0' && begin < end) {
        // todo - get rid of newlines
        Radiant::info("%s%s: \0", traceName, op);
        begin++;
      }
    }
  }

  static void closePipe(int pipe[2])
  {
    for(int i = 0; i < 2; ++i) {
      if(pipe[i] >= 0) {
        ::close(pipe[i]);
        pipe[i] = -1;
      }
    }
  }

  bool SerialPort::close()
  {
    if(m_fd < 0)
      return false;
    ::close(m_fd);
    m_fd = -1;

    closePipe(m_readInterruptPipe);
    closePipe(m_writeInterruptPipe);

    return true;
  }

  int SerialPort::write(const void * buf, int bytes)
  {
    int r = ::write(m_fd, buf, bytes);
    if(r > 0 && m_traceName != nullptr) {
      printBuffer((const char*)buf, r, "<", m_traceName);
    }
    return r;
  }

  SerialPort::WaitStatus SerialPort::wait(int events, double timeoutSecs, int pipe)
  {
    struct pollfd fds[2];
    memset(fds, 0, sizeof(fds));
    fds[0].fd = pipe;
    fds[0].events = POLLIN;
    fds[1].fd = fd();
    fds[1].events = events;
    int ret = poll(fds, 2, std::max<int>(1, timeoutSecs*1000));
    if (ret == -1) {
      return WaitStatus::Error;
    }

    static char buffer[64];
    if (ret > 0 && (fds[0].revents & POLLIN) == POLLIN) {
      int r = 0;
      do {
        r = ::read(pipe, buffer, sizeof(buffer));
      } while(r > 0);
      return WaitStatus::Interrupt;
    }

    bool expected = ret == 1 && (fds[1].revents & events) == events;
    return expected ? WaitStatus::Ok : WaitStatus::Error;
  }

  template<class T>
  static void safeset(T *outp, const T &value)
  {
    if(outp != nullptr) {
      *outp = value;
    }
  }

  int SerialPort::write(const char *buf, int bytes, WriteStatus *status)
  {
    safeset(status, WriteStatus::Ok);
    int written = 0;
    do {
      int r = write(buf + written, bytes - written);
      if (r == 0 || (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        safeset(status, WriteStatus::WouldBlock);
        return written;
      } else if (r < 0) {
        Radiant::error("Failed to write to serial port: %s", strerror(errno));
        safeset(status, WriteStatus::WriteError);
        return written;
      } else {
        written += r;
      }
    } while(written < bytes);
    return written;
  }

  int SerialPort::blockingWrite(const QByteArray &buffer,
                                double timeoutSeconds,
                                bool *ok)
  {
    Timer timer;
    int written = 0;
    safeset(ok, true);
    while(isOpen()) {
      if(buffer.size() == written) {
        return written;
      }
      double timeRemaining = timeoutSeconds - timer.time();
      if(timeRemaining < 0) {
        return written;
      }
      WaitStatus waitStatus = wait(POLLOUT, timeRemaining, m_writeInterruptPipe[0]);
      switch(waitStatus) {
      case WaitStatus::Interrupt:
        return written;
      case WaitStatus::Error:
        safeset(ok, false);
        return written;
      }
      do {
        WriteStatus status;
        int r = write(buffer.data() + written,
                      buffer.size() - written,
                      &status);
        switch(status) {
        case WriteStatus::Ok:
          written += r;
          break;
        case WriteStatus::WouldBlock:
          break;
        case WriteStatus::WriteError:
          safeset(ok, false);
          return written;
        }
      } while(written < buffer.size());
    }
    return written;
  }

  int SerialPort::writeByte(uint8_t byte)
  {
    return write( & byte, 1);
  }

  int SerialPort::read(void * buf, int bytes, bool waitfordata)
  {
    if(!waitfordata) {
      error("SerialPort::read # !waitfordata nor supported yet.");
    }
    int r = ::read(m_fd, buf, bytes);
    if(r > 0 && m_traceName != nullptr) {
      printBuffer((const char*)buf, r, ">", m_traceName);
    }
    return r;
  }

  bool SerialPort::blockingRead(QByteArray *output, double timeoutSeconds)
  {
    assert(output != nullptr);
    WaitStatus waitStatus = wait(POLLIN, timeoutSeconds, m_readInterruptPipe[0]);
    switch(waitStatus) {
    case WaitStatus::Interrupt:
      return true;
    case WaitStatus::Error:
      return false;
    case WaitStatus::Ok:
      break;
    }
    char buffer[256];
    while(true) {
      errno = 0;
      int r = read(buffer, 256);
      if (r > 0) {
        output->append(buffer, r);
      } else if (r == 0 || (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
        break;
      } else if (r < 0) {
        return false;
      }
    }
    return true;
  }

  void SerialPort::interrupt(int fd)
  {
    if(fd >= 0) {
      int r = ::write(fd, "!", 1);
      if(r < 0) {
        Radiant::error("Error writing to interrupt pipe: %s", strerror(errno));
      }
    }
  }

  void SerialPort::interruptRead()
  {
    interrupt(m_readInterruptPipe[1]);
  }

  void SerialPort::interruptWrite()
  {
    interrupt(m_writeInterruptPipe[1]);
  }

  bool SerialPort::isOpen() const
  {
    return m_fd >= 0;
  }

}

#endif
