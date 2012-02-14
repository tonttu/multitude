#include "VM1.hpp"

#include "ColorCorrection.hpp"
#include "BGThread.hpp"

#include <Radiant/SerialPort.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <QRegExp>
#include <QStringList>
#include <QFile>

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

  QString VM1::info()
  {
    Radiant::Guard g(s_vm1Mutex);

    bool ok;
    Radiant::SerialPort & port = open(ok);
    if(!ok) return "";

    char buffer[256];
    while(port.read(buffer, 256) > 0) {}

    QByteArray ba("i");
    if(writeAll(ba.data(), ba.size(), port, 300) != ba.size())
      return "";

    QByteArray res;
    for(;;) {
      /// @todo Add timeout reading to SerialPort! This is just very temporary stupid hack
      Radiant::Sleep::sleepMs(4);
      int r = port.read(buffer, 256);
      if(r > 0) {
        res.append(buffer, r);
      } else break;
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
    if(!m_port.isOpen() && !m_port.open(dev.toUtf8().data(), false, false, 115200, 8, 1, 10000)) {
      Radiant::error("Failed to open %s", dev.toUtf8().data());
      ok = false;
    } else {
      ok = true;
    }
    return m_port;
  }

}
