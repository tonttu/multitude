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

#include <Radiant/SerialPort.hpp>

#include <Radiant/Trace.hpp>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

namespace Radiant
{

  SerialPort::SerialPort()
  : m_fd(-1)
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
    
#if 0
    opts.c_iflag = (IGNBRK | IGNPAR);  /* Ignore break & parity errs */
    opts.c_oflag = 0;                  /* Raw output, leave tabs alone */
    opts.c_lflag = 0;              /* Raw input (no KILL, etc.), no echo */
#endif

    if(tcsetattr(m_fd, TCSANOW, & opts) < 0) {
      Radiant::error("%s # Failed to set TTY parameters (%s)", fname, strerror(errno));
      errno = 0;
      return false;
    }

    return true;
  }

  bool SerialPort::close()
  {
    if(m_fd < 0)
      return false;

    ::close(m_fd);

    m_fd = -1;

    return true;
  }

  int SerialPort::write(const void * buf, int bytes)
  {
    return ::write(m_fd, buf, bytes);
  }

  int SerialPort::writeByte(uint8_t byte)
  {
    return write( & byte, 1);
  }

  int SerialPort::read(void * buf, int bytes)
  {
    return ::read(m_fd, buf, bytes);
  }

  bool SerialPort::isOpen() const
  {
    return m_fd >= 0;
  }

}
