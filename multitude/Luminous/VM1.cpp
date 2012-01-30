#include "VM1.hpp"

#include "ColorCorrection.hpp"
#include "BGThread.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#ifdef RADIANT_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace {
  QString findVM1() {
#ifdef RADIANT_LINUX
    return "/dev/ttyVM1";
#else
    return "";
#endif
  }

  Radiant::Mutex s_vm1Mutex;

  int writeAll(const char * data, int len, Radiant::SerialPort & port, int timeoutMS)
  {
    int written = 0;
    do {
      int r = port.write(data + written, len - written);
      if(r < 0) {
        int e = errno;
        if(e == EAGAIN && timeoutMS > 0) {
          Radiant::Sleep::sleepMs(Nimble::Math::Max(1, timeoutMS));
          --timeoutMS;
          continue;
        } else {
#ifdef RADIANT_LINUX
          Radiant::error("Failed to write to VM1 serial port: %s", strerror(errno));
#else
          Radiant::error("Failed to write to VM1 serial port");
#endif
          return -1;
        }
      }
      written += r;
    } while (len - written > 0);
    return written;
  }

  class BGWriter : public Luminous::Task
  {
  public:
    BGWriter(Luminous::VM1 & vm1) : m_vm1(vm1)
    {
      /// At least VM1 Firmware version 2.4 turns off the "Color gamma" mode
      /// while reading the new color table, meaning that if we always
      /// immediately set the new color table when moving a slider in GUI,
      /// all we get is a blinking screen without the color correction.
      /// .. with this delay we get slower blinking screen with color correction
      scheduleFromNowSecs(0.1);
    }

    void doTask()
    {
      setFinished();
      QByteArray ba = m_vm1.takeData();
      if(ba.isEmpty()) return;

      Radiant::Guard g(s_vm1Mutex);

      bool ok;
      Radiant::SerialPort & port = m_vm1.open(ok);
      if(ok)
        writeAll(ba.data(), ba.size(), port, 300);
    }

  private:
    Luminous::VM1 & m_vm1;
  };
}

namespace Luminous
{

  VM1::VM1()
  {
  }

  bool VM1::detected() const
  {
#ifdef RADIANT_LINUX
    struct stat tmp;
    return stat(findVM1().toUtf8().data(), &tmp) == 0 && (tmp.st_mode & S_IFCHR);
#else
    return QFile::exists(findVM1());
#endif
  }

  void VM1::setColorCorrection(const ColorCorrection & cc)
  {
    QByteArray ba;
    ba.reserve(256*3 + 2);

    // Load gamma
    ba += 'd';

    Nimble::Vector3T<uint8_t> tmp[256];
    cc.fillAsBytes(tmp);
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].x;
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].y;
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].z;

    // Enable gamma
    ba += 'g';

    Radiant::Guard g(m_dataMutex);
    bool createTask = m_data.isEmpty();
    m_data = ba;
    if(createTask)
      Luminous::BGThread::instance()->addTask(new BGWriter(*this));
  }

  QByteArray VM1::takeData()
  {
    QByteArray ba;
    Radiant::Guard g(m_dataMutex);
    std::swap(ba, m_data);
    return ba;
  }

  Radiant::SerialPort & VM1::open(bool & ok)
  {
    QString dev = findVM1();
    if(!m_port.isOpen() && !m_port.open(dev.toUtf8().data(), false, false, 115200, 8, 0, 0)) {
      Radiant::error("Failed to open %s", dev.toUtf8().data());
      ok = false;
    } else {
      ok = true;
    }
    return m_port;
  }

}
