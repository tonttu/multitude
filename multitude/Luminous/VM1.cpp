#include "VM1.hpp"

#include "ColorCorrection.hpp"
#include "BGThread.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <QRegExp>
#include <QStringList>
#include <QFile>

#ifdef RADIANT_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace
{
  static int writeAll(const char * data, int len, Radiant::SerialPort & port, int timeoutMS)
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
}

namespace Luminous
{
  static Radiant::Mutex s_vm1Mutex;

  class VM1::D : public Luminous::Task, public std::enable_shared_from_this<VM1::D>
  {
  public:
    D();
    void sendCommand(const QByteArray & ba);

    QString findVM1();

    virtual void doTask() OVERRIDE;

    bool open();

  public:
    QByteArray m_data;
    Radiant::Mutex m_dataMutex;
    Radiant::SerialPort m_port;

    int m_errors;
  };

  VM1::D::D()
    : m_errors(0)
  {
    setFinished();
    /// VM1 Firmware version 2.4 turns off the "Color gamma" mode
    /// while reading the new color table, meaning that if we always
    /// immediately set the new color table when moving a slider in GUI,
    /// all we get is a blinking screen without the color correction.
    /// .. with this delay we get slower blinking screen with color correction
    //scheduleFromNowSecs(0.1);

    /// That is fixed in 2.6, we could check version numbers, but that could
    /// cause some extra latency. For now just assume that we have a new
    /// enough version.
  }

  QString VM1::D::findVM1()
  {
#ifdef RADIANT_LINUX
    return "/dev/ttyVM1";
#else
    return "";
#endif
  }

  void VM1::D::doTask()
  {
    {
      Radiant::Guard g(m_dataMutex);
      if (m_data.isEmpty()) {
        setFinished();
        return;
      }
    }

    if (!open()) {
      ++m_errors;
      if (m_errors > 5) {
        Radiant::error("VM1 # Failed to open VM1");
        m_errors = 0;
        Radiant::Guard g(m_dataMutex);
        m_data.clear();
        setFinished();
        return;
      }
      scheduleFromNowSecs(0.5 + m_errors * 0.2);
      return;
    }

    QByteArray ba;
    {
      Radiant::Guard g(m_dataMutex);
      if (m_data.isEmpty()) {
        setFinished();
        return;
      }
      std::swap(ba, m_data);
    }

    {
      Radiant::Guard g(s_vm1Mutex);
      writeAll(ba.data(), ba.size(), m_port, 300);
    }
    m_errors = 0;

    Radiant::Guard g(m_dataMutex);
    if (m_data.isEmpty()) {
      setFinished();
    } else {
      scheduleFromNowSecs(0);
    }
  }

  bool VM1::D::open()
  {
    if (m_port.isOpen())
      return true;

    QString dev = findVM1();
    if (dev.isEmpty())
      return false;

    if (!m_port.open(dev.toUtf8().data(), false, false, 115200, 8, 1, 10000))
      return false;

    return true;
  }

  void VM1::D::sendCommand(const QByteArray & ba)
  {
    Radiant::Guard g(m_dataMutex);
    m_data += ba;
    if (state() == DONE) {
      setState(WAITING);
      Luminous::BGThread::instance()->addTask(shared_from_this());
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  VM1::VM1()
    : m_d(new D())
  {}

  VM1::~VM1()
  {}

  bool VM1::detected() const
  {
#ifdef RADIANT_LINUX
    struct stat tmp;
    return stat(m_d->findVM1().toUtf8().data(), &tmp) == 0 && (tmp.st_mode & S_IFCHR);
#else
    return QFile::exists(m_d->findVM1());
#endif
  }

  void VM1::setColorCorrection(const ColorCorrection & cc)
  {
    QByteArray ba;
    ba.reserve(256*3 + 2);

    // Load gamma
    ba += 'd';

    std::vector<Nimble::Vector3ub> tmp(256);
    cc.fill(tmp);
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].x;
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].y;
    for(int i = 0; i < 256; ++i)
      ba += tmp[i].z;

    // Enable gamma
    ba += 'g';

    m_d->sendCommand(ba);
  }

  void VM1::setLCDPower(bool enable)
  {
    m_d->sendCommand(enable ? "o" : "f");
  }

  void VM1::setLogoTimeout(int timeout)
  {
    QByteArray ba;
    ba.reserve(2);

    ba += 'x';
    ba += QByteArray::number(Nimble::Math::Clamp(timeout, 1, 99));
    ba += '\n';

    m_d->sendCommand(ba);
  }

  void VM1::setVideoAutoselect()
  {
    m_d->sendCommand("a");
  }

  void VM1::setVideoInput(int input)
  {
    if(input <= 0 || input > 4)
      Radiant::error("VM1::setVideoInput # allowed inputs [1-4]");
    else
      m_d->sendCommand(QByteArray::number(input));
  }

  void VM1::setVideoInputPriority(int input)
  {
    QByteArray ba;

    if(input == 1)
      ba += 'y';
    else if (input == 2)
      ba += 'u';
    else
      Radiant::error("VM1::setVideoInputPriority # allowed inputs [1-2]");

    if(ba.length() > 0)
      m_d->sendCommand(ba);
  }

  void VM1::enableGamma(bool state)
  {
    m_d->sendCommand(state ? "g" : "c");
  }

  QString VM1::info()
  {
    /// @todo there should be a timeout parameter
    if (!m_d->open()) {
      if (detected()) {
        Radiant::Sleep::sleepS(1);
        if (!m_d->open())
          return QString();
      } else {
        return QString();
      }
    }
    Radiant::Guard g(s_vm1Mutex);

    char buffer[256];
    while (m_d->m_port.read(buffer, 256) > 0) {}

    QByteArray ba("i");
    if(writeAll(ba.data(), ba.size(), m_d->m_port, 300) != ba.size())
      return QString();

    QByteArray res;
    int prev = 0;
    for (int i = 0; i < 100; ++i) {
      /// @todo Add timeout reading to SerialPort! This is just very temporary stupid hack
      int r = m_d->m_port.read(buffer, 256);
      if(r > 0) {
        res.append(buffer, r);
        prev = r;
      } else if (r == 0 || errno != EAGAIN) {
        break;
      } else {
        if (res.size() > 0 && prev == 0)
          break;
        prev = 0;
        Radiant::Sleep::sleepMs(4);
      }
    }
    return res;
  }

  QMap<QString, QString> VM1::parseInfo(const QString & info)
  {
    QRegExp header("Info");
    QRegExp version("Firmware version (.+)");
    QRegExp board("Board revision (.+)");
    QRegExp autosel("Autoselect is (on|off)");
    QRegExp priority ("(.+) has priority");
    QRegExp notConnected("(.+) not connected");
    QRegExp detected("(.+) detected");
    QRegExp active("(.+) active");
    QRegExp pixelsLines("Total pixels: (\\d+) Actives pixels: (\\d+) Total lines: (\\d+) Actives lines: (\\d+)");
    QRegExp uptime("Operation time (\\d+) hours and (\\d+) minutes");
    QRegExp screensaver("Screensaver time (\\d+) minutes");
    QRegExp temp("Temperature (\\d+) degrees");
    QRegExp src("Video source is (.+)");
    QRegExp totalPixels("Total pixels: (\\d+)");
    QRegExp activePixels("Actives? pixels: (\\d+)");
    QRegExp totalLines("Total lines: (\\d+)");
    QRegExp activeLines("Actives? lines: (\\d+)");
    QRegExp colorCorrection("Color gamma is (on|off)");
    QRegExp sdram("SDRAM status (\\d+) / (\\d+)");

    QRegExp split("\\s*[\\r\\n]+\\s*", Qt::CaseInsensitive, QRegExp::RegExp2);

    QMap<QString, QString> map;

    QSet<QString> outputs;
    foreach(const QString & line, info.split(split, QString::SkipEmptyParts)) {
      if(header.exactMatch(line)) {
        continue;
      } else if(version.exactMatch(line)) {
        map["version"] = version.cap(1);
      } else if(board.exactMatch(line)) {
        map["board-revision"] = board.cap(1);
      } else if(autosel.exactMatch(line)) {
        map["autoselect"] = autosel.cap(1) == "on" ? "1" : "";
      } else if(priority.exactMatch(line)) {
        outputs << priority.cap(1);
        map["priority"] = priority.cap(1);
      } else if(notConnected.exactMatch(line)) {
        map[notConnected.cap(1)] = "not connected";
        outputs << notConnected.cap(1);
      } else if(detected.exactMatch(line)) {
        map[detected.cap(1)] = "detected";
        outputs << detected.cap(1);
      } else if(active.exactMatch(line)) {
        map[active.cap(1)] = "active";
        outputs << active.cap(1);
      } else if(pixelsLines.exactMatch(line)) {
        map["total-pixels"] = pixelsLines.cap(1);
        map["active-pixels"] = pixelsLines.cap(2);
        map["total-lines"] = pixelsLines.cap(3);
        map["active-lines"] = pixelsLines.cap(4);
      } else if(uptime.exactMatch(line)) {
        map["uptime"] = QString::number(uptime.cap(1).toInt() * 60 + uptime.cap(2).toInt());
      } else if(screensaver.exactMatch(line)) {
        map["screensaver"] = screensaver.cap(1);
      } else if(temp.exactMatch(line)) {
        map["temperature"] = temp.cap(1);
      } else if(src.exactMatch(line)) {
        map["source"] = src.cap(1);
        outputs << src.cap(1);
      } else if(totalPixels.exactMatch(line)) {
        map["total-pixels"] = totalPixels.cap(1);
      } else if(activePixels.exactMatch(line)) {
        map["active-pixels"] = activePixels.cap(1);
      } else if(totalLines.exactMatch(line)) {
        map["total-lines"] = totalLines.cap(1);
      } else if(activeLines.exactMatch(line)) {
        map["active-lines"] = activeLines.cap(1);
      } else if(colorCorrection.exactMatch(line)) {
        map["color-correction"] = activeLines.cap(1) == "on" ? "1" : "";
      } else if(sdram.exactMatch(line)) {
        map["sdram-status"] = sdram.cap(1).toInt();
        map["sdram-total"] = sdram.cap(2).toInt();
      } else {
        Radiant::warning("VM1::parseInfo # Failed to parse line '%s'", line.toUtf8().data());
      }
    }
    if(!outputs.isEmpty())
      map["outputs"] = QStringList(outputs.toList()).join("\n");
    return map;
  }
}
