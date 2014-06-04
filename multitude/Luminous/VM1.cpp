/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "VM1.hpp"

#include "ColorCorrection.hpp"

#include <Radiant/BGThread.hpp>
#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <QRegExp>
#include <QStringList>
#include <QFile>
#include <QSettings>
#include <QSystemSemaphore>

#ifdef RADIANT_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace
{
  static QSystemSemaphore s_vm1Lock("MultiTouchVM1", 1);
  static bool s_enabled = true;

  class VM1Guard
  {
  public:
    VM1Guard() { s_vm1Lock.acquire(); }
    ~VM1Guard() { s_vm1Lock.release(); }
  };

  static int writeAll(const char * data, int len, Radiant::SerialPort & port, int timeoutMS)
  {
    int written = 0;
    do {
      int r = port.write(data + written, len - written);
      if(r < 0) {
        int e = errno;
        if(e == EAGAIN && timeoutMS > 0) {
          Radiant::Sleep::sleepMs(std::max(1, timeoutMS));
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

  void clearReadBuffer(Radiant::SerialPort & port, int timeoutMs)
  {
    char buffer[256];
    Radiant::Sleep::sleepMs(timeoutMs);
    while (port.read(buffer, sizeof(buffer)) > 0)
      Radiant::Sleep::sleepMs(timeoutMs);
  }

  bool checkForVM1(Radiant::SerialPort & port, int timeoutMs)
  {
    clearReadBuffer(port, timeoutMs);

    QByteArray buffer(2048, '\0');

    if (port.write("?", 1) != 1)
      return false;

    // we don't want partial response, so we sleep
    Radiant::Sleep::sleepMs(timeoutMs);

    int bytes = port.read(buffer.data(), buffer.size(), true);

    if (bytes > 0) {
      buffer.resize(bytes);
      if (buffer.contains("VM1") || buffer.contains("LVDS power")) {
        clearReadBuffer(port, timeoutMs);
        return true;
      }
    }

    return false;
  }

#ifdef RADIANT_WINDOWS

  QString scanPorts(int timeoutMs)
  {
    VM1Guard g;
    foreach (const QString dev, Radiant::SerialPort::scan()) {
      Radiant::SerialPort port;
      if (port.open(dev.toUtf8().data(), false, false, 115200, 8, 1, timeoutMs*1000)) {
        if(checkForVM1(port, timeoutMs)) {
          return dev;
        }
      }
    }
    return QString();
  }

  static Radiant::Mutex s_scanMutex;
  QString discover()
  {
    const int retries = 10;
    QSettings settings("SOFTWARE\\MultiTouch\\MTSvc", QSettings::NativeFormat);
    QString dev = settings.value("VM1").toString();
    if (dev.isEmpty()) {
      Radiant::Guard g(s_scanMutex);
      settings.sync();
      dev = settings.value("VM1").toString();
      if (!dev.isEmpty())
        return dev;

      static int retry = 0;
      if (retry > retries)
        return QString();
      dev = scanPorts(retry++ == 0 ? 500 : 30);
      if (!dev.isEmpty())
        settings.setValue("VM1", dev);
    }
    return dev;
  }

  void vm1Opened(bool ok)
  {
    static int errors = 0;
    if (ok) {
      errors = 0;
    } else {
      if (++errors == 5) {
        Radiant::warning("Too many errors, clearing VM1 device name");
        QSettings settings("SOFTWARE\\MultiTouch\\MTSvc", QSettings::NativeFormat);
        settings.setValue("VM1", "");
      }
    }
  }

#else
  void vm1Opened(bool) {}
#endif

}

namespace Luminous
{
  class VM1::D : public Radiant::Task
  {
  public:
    D();
    void sendCommand(const QByteArray & ba);
    void sendColorCorrection(const QByteArray & ba);

    QString findVM1();

    virtual void doTask() OVERRIDE;

    bool open();
    QByteArray readInfo();

  public:
    Radiant::Mutex m_dataMutex;
    QByteArray m_data;
    QByteArray m_dataColorCorrection;
    Radiant::SerialPort m_port;

    Radiant::Mutex m_infoMutex;
    QByteArray m_info;
    Radiant::TimeStamp m_infoTimeStamp;

    int m_errors;

    static std::weak_ptr<VM1::D> s_d;
  };
  std::weak_ptr<VM1::D> VM1::D::s_d;

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
    if (!s_enabled) return QString();
#ifdef RADIANT_LINUX
    return "/dev/ttyVM1";
#elif defined(RADIANT_WINDOWS)
    return discover();
#else
    return "";
#endif
  }

  void VM1::D::doTask()
  {
    if (!s_enabled) {
      setFinished();
      return;
    }
    {
      Radiant::Guard g(m_dataMutex);
      if (m_data.isEmpty() && m_dataColorCorrection.isEmpty()) {
        setFinished();
        return;
      }
    }

    VM1Guard g;
    if (!open()) {
      ++m_errors;
      if (m_errors > 5) {
        Radiant::error("VM1 # Failed to open VM1");
        m_errors = 0;
        Radiant::Guard g2(m_dataMutex);
        m_data.clear();
        m_dataColorCorrection.clear();
        setFinished();
        return;
      }
      scheduleFromNowSecs(0.5 + m_errors * 0.2);
      return;
    }

    QByteArray ba;
    {
      Radiant::Guard g2(m_dataMutex);
      if (m_data.isEmpty() && m_dataColorCorrection.isEmpty()) {
        setFinished();
        return;
      }
      if (!m_data.isEmpty())
        std::swap(ba, m_data);
      else
        std::swap(ba, m_dataColorCorrection);
    }

    writeAll(ba.data(), ba.size(), m_port, 500);
    m_errors = 0;

    Radiant::Guard g2(m_dataMutex);
    if (m_data.isEmpty() && m_dataColorCorrection.isEmpty()) {
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

    if (!m_port.open(dev.toUtf8().data(), false, false, 115200, 8, 1, 30000)) {
      vm1Opened(false);
      return false;
    } else {
      // Port was successfully opened - actually check that there is a VM1 on the other end
      bool isVM1 = checkForVM1(m_port, 1000);

      if(!isVM1)
        m_port.close();

      vm1Opened(isVM1);
      return isVM1;
    }
  }

  QByteArray VM1::D::readInfo()
  {
    if (!s_enabled) return QByteArray();
    VM1Guard g;
    /// @todo there should be a timeout parameter
    if (!open()) {
      Radiant::Sleep::sleepS(1);
      if (!open())
        return QByteArray();
    }

    clearReadBuffer(m_port, 1);
    /// Empty the read buffer
    char buffer[256];

    const QByteArray ba("i");
    if (writeAll(ba.data(), ba.size(), m_port, 300) != ba.size()) {
      m_port.close();
      return QByteArray();
    }

    QByteArray res;
    int prev = 0;
    for (int i = 0; i < 100; ++i) {
      /// @todo Add timeout reading to SerialPort! This is just very temporary stupid hack
      errno = 0;
      int r = m_port.read(buffer, sizeof(buffer));
#ifdef RADIANT_WINDOWS
      if (r == 0) {
        DWORD err = GetLastError();
        if (err == ERROR_SUCCESS || err == ERROR_IO_PENDING) {
          if (res.size() > 0 && prev == 0)
            break;

          prev = 0;
          Radiant::Sleep::sleepMs(30);
          continue;
        }
        break;
      } else
#endif
      if (r > 0) {
        res.append(buffer, r);
        prev = r;
      } else if (r == 0 || errno != EAGAIN) {
        if (res.size() == 0 && i == 0) {
          m_port.close();
          if (!open())
            break;
          if (writeAll(ba.data(), ba.size(), m_port, 300) != ba.size())
            break;
          continue;
        }
        break;
      } else {
        if (res.size() > 0 && prev == 0)
          break;
        prev = 0;
        Radiant::Sleep::sleepMs(10);
      }
    }

    return res;
  }

  void VM1::D::sendCommand(const QByteArray & ba)
  {
    if (!s_enabled) return;
    Radiant::Guard g(m_dataMutex);
    m_data += ba;
    if (state() == DONE) {
      setState(WAITING);
      Radiant::BGThread::instance()->addTask(shared_from_this());
    }
  }

  void VM1::D::sendColorCorrection(const QByteArray & ba)
  {
    if (!s_enabled) return;
    Radiant::Guard g(m_dataMutex);
    m_dataColorCorrection = ba;
    if (state() == DONE) {
      setState(WAITING);
      Radiant::BGThread::instance()->addTask(shared_from_this());
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  VM1::VM1()
    : m_d(D::s_d.lock())
  {
    if (!m_d) {
      VM1Guard g;
      m_d = D::s_d.lock();
      if (!m_d) {
        m_d.reset(new D());
        D::s_d = m_d;
      }
    }
    eventAddOut("info-updated");
  }

  VM1::~VM1()
  {}

  bool VM1::detected(bool useCachedValue, Radiant::TimeStamp * ts)
  {
    return !info(useCachedValue, ts).isEmpty();
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

    m_d->sendColorCorrection(ba);
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

  QByteArray VM1::rawInfo(bool useCachedValue, Radiant::TimeStamp * ts)
  {
    {
      Radiant::Guard g(m_d->m_infoMutex);
      if (useCachedValue && m_d->m_infoTimeStamp != Radiant::TimeStamp()) {
        if (ts) *ts = m_d->m_infoTimeStamp;
        return m_d->m_info;
      }
    }

    QByteArray ba = m_d->readInfo();

    {
      Radiant::Guard g(m_d->m_infoMutex);
      m_d->m_infoTimeStamp = Radiant::TimeStamp::currentTime();
      m_d->m_info = ba;
      if (ts) *ts = m_d->m_infoTimeStamp;
    }
    eventSend("info-updated");
    return ba;
  }

  VM1::Info VM1::info(bool useCachedValue, Radiant::TimeStamp * ts)
  {
    return parseInfo(rawInfo(useCachedValue, ts));
  }

  VM1::Info VM1::parseInfo(const QByteArray & data)
  {
    VM1::Info info;

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
    QRegExp temp("Temperature (-?\\d+) degrees");
    QRegExp src("Video source is (.+)");
    QRegExp totalPixels("Total pixels: (\\d+)");
    QRegExp activePixels("Actives? pixels: (\\d+)");
    QRegExp totalLines("Total lines: (\\d+)");
    QRegExp activeLines("Actives? lines: (\\d+)");
    QRegExp colorCorrection("Color gamma is (on|off)");
    // Some VM1 firmware version changed this
    //   SDRAM status \d+ / \d+
    // to:
    //   SDRAM status: \d+ / eye: \d+
    //
    // At least VM1 version 3.3 uses the latter format.
    QRegExp sdram("SDRAM status:? (\\d+) / (eye: )?(\\d+)");

    for (QByteArray lineRaw: data.split('\n')) {
      lineRaw = lineRaw.trimmed();
      if (lineRaw.isEmpty())
        continue;
      const QString line = lineRaw;

      if (header.exactMatch(line)) {
        info.header = true;
      } else if (version.exactMatch(line)) {
        info.version = version.cap(1);
      } else if (board.exactMatch(line)) {
        info.boardRevision = board.cap(1);
      } else if (autosel.exactMatch(line)) {
        info.autoselect = autosel.cap(1) == "on" ? Info::INFO_TRUE : Info::INFO_FALSE;
      } else if (priority.exactMatch(line)) {
        info.sources[priority.cap(1)] = std::max(info.sources[priority.cap(1)], Info::STATUS_UNKNOWN);
        info.prioritySource = priority.cap(1);
      } else if (notConnected.exactMatch(line)) {
        info.sources[notConnected.cap(1)] = std::max(info.sources[notConnected.cap(1)], Info::STATUS_NOT_CONNECTED);
      } else if (detected.exactMatch(line)) {
        info.sources[detected.cap(1)] = std::max(info.sources[detected.cap(1)], Info::STATUS_DETECTED);
      } else if (active.exactMatch(line)) {
        info.sources[active.cap(1)] = std::max(info.sources[active.cap(1)], Info::STATUS_ACTIVE);
      } else if (pixelsLines.exactMatch(line)) {
        info.totalPixels = pixelsLines.cap(1).toInt();
        info.activePixels = pixelsLines.cap(2).toInt();
        info.totalLines = pixelsLines.cap(3).toInt();
        info.activeLines = pixelsLines.cap(4).toInt();
      } else if (uptime.exactMatch(line)) {
        info.uptimeMins = uptime.cap(1).toInt() * 60 + uptime.cap(2).toInt();
      } else if (screensaver.exactMatch(line)) {
        info.screensaverMins = screensaver.cap(1).toInt();
      } else if (temp.exactMatch(line)) {
        info.temperature = temp.cap(1).toInt();
      } else if (src.exactMatch(line)) {
        info.videoSource = src.cap(1);
        info.sources[src.cap(1)] = std::max(info.sources[src.cap(1)], Info::STATUS_ACTIVE);
      } else if (totalPixels.exactMatch(line)) {
        info.totalPixels = totalPixels.cap(1).toInt();
      } else if (activePixels.exactMatch(line)) {
        info.activePixels = activePixels.cap(1).toInt();
      } else if (totalLines.exactMatch(line)) {
        info.totalLines = totalLines.cap(1).toInt();
      } else if (activeLines.exactMatch(line)) {
        info.activeLines = activeLines.cap(1).toInt();
      } else if (colorCorrection.exactMatch(line)) {
        info.colorCorrectionEnabled = colorCorrection.cap(1) == "on" ? Info::INFO_TRUE : Info::INFO_FALSE;
      } else if (sdram.exactMatch(line)) {
        info.sdramStatus = sdram.cap(1).toInt();
        info.sdramTotal = sdram.cap(3).toInt();
      } else {
        info.extraLines << lineRaw;
        Radiant::warning("VM1::parseInfo # Failed to parse line '%s'", line.toUtf8().data());
      }
    }

    return info;
  }

  bool VM1::enabled()
  {
    return s_enabled;
  }

  void VM1::setEnabled(bool enabled)
  {
    s_enabled = enabled;
  }

  DEFINE_SINGLETON(VM1)
}
