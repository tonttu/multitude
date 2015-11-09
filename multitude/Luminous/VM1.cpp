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

#include <atomic>

#include <Radiant/BGThread.hpp>
#include <Radiant/SerialPort.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Timer.hpp>

#include <Valuable/AttributeBool.hpp>
#include <Valuable/AttributeEnum.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeString.hpp>
#include <Valuable/AttributeStringList.hpp>
#include <Valuable/AttributeTimeStamp.hpp>

#include <QDir>
#include <QSettings>
#include <deque>

#ifdef RADIANT_UNIX
#include <string.h>
#endif

namespace
{
  static bool s_enabled = true;

  Valuable::EnumNames s_maybe[] = {{"false", Luminous::VM1::MAYBE_FALSE},
                                   {"true", Luminous::VM1::MAYBE_TRUE},
                                   {"unknown", Luminous::VM1::MAYBE_UNKNOWN},
                                   {0, 0}};

  Valuable::EnumNames s_src[] = {{"false", Luminous::VM1::SOURCE_NONE},
                                 {"external-dvi", Luminous::VM1::SOURCE_EXTERNAL_DVI},
                                 {"internal-dvi", Luminous::VM1::SOURCE_INTERNAL_DVI},
                                 {"test-image", Luminous::VM1::SOURCE_TEST_IMAGE},
                                 {"logo", Luminous::VM1::SOURCE_LOGO},
                                 {0, 0}};

  Valuable::EnumNames s_status[] = {{"unknown", Luminous::VM1::STATUS_UNKNOWN},
                                    {"not-connected", Luminous::VM1::STATUS_NOT_CONNECTED},
                                    {"detected", Luminous::VM1::STATUS_DETECTED},
                                    {"active", Luminous::VM1::STATUS_ACTIVE},
                                    {0, 0}};
}  // unnamed namespace

namespace Luminous
{
  class VM1::D : public Radiant::Thread
  {
  public:
    D(VM1 * host);

    virtual void childLoop();

    void sleep(double seconds);

    QStringList findVM1();

    bool open();
    void opened(bool ok);
    void closePort();

    void writeColorCorrection();
    void runTasks();

    void queueWrite(const QByteArray & data);

    QList<QByteArray> takeLines();
    void processBuffer();
    void scheduleUpdate();

  public:
    VM1 * m_host;

    Valuable::AttributeBool m_connected;
    Valuable::AttributeString m_version;
    Valuable::AttributeString m_boardRevision;
    Valuable::AttributeEnumT<VM1::Maybe> m_autoSelect;
    Valuable::AttributeEnumT<VM1::VideoSource> m_priorityVideoSource;
    Valuable::AttributeEnumT<VM1::SourceStatus> m_statusExternalDVI;
    Valuable::AttributeEnumT<VM1::SourceStatus> m_statusInternalDVI;
    Valuable::AttributeEnumT<VM1::VideoSource> m_activeVideoSource;
    Valuable::AttributeVector2i m_totalSize;
    Valuable::AttributeVector2i m_activeSize;
    Valuable::AttributeTimeStamp m_bootTime;
    Valuable::AttributeInt m_logoTimeout;
    Valuable::AttributeInt m_temperature;
    Valuable::AttributeTimeStamp m_temperatureTimestamp;
    Valuable::AttributeEnumT<VM1::Maybe> m_colorCorrectionEnabled;
    Valuable::AttributeInt m_sdramStatus;
    Valuable::AttributeInt m_sdramTotal;
    Valuable::AttributeFloat m_frameRate;

    // Write-only value, we really don't know the actual active state
    Valuable::AttributeBool m_lcdPower;

    Valuable::AttributeStringList m_unknownLines;

    Radiant::Mutex m_connectedMutex;
    Radiant::Condition m_connectedCondition;

    bool m_listenersEnabled;

    Radiant::TimeStamp m_startTime;

    bool m_running;
    Radiant::SerialPort m_port;

    Radiant::Mutex m_colorCorrectionMutex;
    QByteArray m_colorCorrection;

    std::deque<VM1Task> m_tasks;
    Radiant::Mutex m_taskMutex;

    QString m_device;
    QStringList m_deviceCandidates;

    QByteArray m_readBuffer;
    Radiant::Mutex m_readBufferMutex;

    QByteArray m_writeBuffer;
    Radiant::Mutex m_writeBufferMutex;

    std::atomic<bool> m_requestReconnect;

    bool m_updateScheduled;

    Radiant::TaskPtr m_infoPoller;

    bool m_useColorCorrectionDelay;

    bool m_parsingHelp;

    int m_headerGeneration;

    QRegExp m_reHeader;
    QRegExp m_reVersion;
    QRegExp m_reBoard;
    QRegExp m_reAutosel;
    QRegExp m_rePriority;
    QRegExp m_reNotConnected;
    QRegExp m_reDetected;
    QRegExp m_reActive;
    QRegExp m_rePixelsLines;
    QRegExp m_reUptime;
    QRegExp m_reScreensaver;
    QRegExp m_reTemp;
    QRegExp m_reSrc;
    QRegExp m_reTotalPixels;
    QRegExp m_reActivePixels;
    QRegExp m_reTotalLines;
    QRegExp m_reActiveLines;
    QRegExp m_reColorCorrection;
    QRegExp m_reSdram;
    QRegExp m_reHelpHeader;
    QRegExp m_reCmdHelpMsg;
    QRegExp m_reHelpMsg;
    QRegExp m_reSelect;
    QRegExp m_reBoot;
    QRegExp m_reInit;
    QRegExp m_reFrameRate;
    QRegExp m_reFailToLock;
    QRegExp m_reClockLost;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  VM1::D::D(VM1 * host)
    : m_host(host),
      m_connected(host, "connected", false),
      m_version(host, "version"),
      m_boardRevision(host, "board-revision"),
      m_autoSelect(host, "auto-select", s_maybe, MAYBE_UNKNOWN),
      m_priorityVideoSource(host, "priority-video-source", s_src, SOURCE_NONE),
      m_statusExternalDVI(host, "status-external-dvi", s_status, STATUS_UNKNOWN),
      m_statusInternalDVI(host, "status-internal-dvi", s_status, STATUS_UNKNOWN),
      m_activeVideoSource(host, "active-video-source", s_src, SOURCE_NONE),
      m_totalSize(host, "total-size"),
      m_activeSize(host, "active-size"),
      m_bootTime(host, "boot-time"),
      m_logoTimeout(host, "logo-timeout", std::numeric_limits<int>::min()),
      m_temperature(host, "temperature", std::numeric_limits<int>::min()),
      m_temperatureTimestamp(host, "temperature-timestamp"),
      m_colorCorrectionEnabled(host, "color-correction-enabled", s_maybe, MAYBE_UNKNOWN),
      m_sdramStatus(host, "sdram-status", std::numeric_limits<int>::min()),
      m_sdramTotal(host, "sdram-total", std::numeric_limits<int>::min()),
      m_frameRate(host, "frame-rate", std::numeric_limits<int>::min()),
      m_lcdPower(host, "lcd-power", true),
      m_unknownLines(host, "unknown-lines"),
      m_listenersEnabled(true),
      m_running(true),
      m_updateScheduled(false),
      m_useColorCorrectionDelay(false),
      m_parsingHelp(false),
      m_headerGeneration(0),
      m_reHeader("(Info|VM1)"),
      m_reVersion("Firmware version (.+)"),
      m_reBoard("Board revision (.+)"),
      m_reAutosel("Autoselect is (on|off)"),
      m_rePriority("(DVI[12]) has priority"),
      m_reNotConnected("(DVI[12]) (disconnected|not connected)"),
      m_reDetected("(DVI[12]) detected"),
      m_reActive("(DVI[12]) active"),
      m_rePixelsLines("Total pixels: (\\d+) Actives pixels: (\\d+) Total lines: (\\d+) Actives lines: (\\d+)"),
      m_reUptime("Operation time (\\d+) hours and (\\d+) minutes"),
      m_reScreensaver("Screensaver time (?:set to )?(\\d+) minutes"),
      m_reTemp("Temperature (-?\\d+) degrees"),
      m_reSrc("Video source is (DVI1|DVI2|colorbar|logo)"),
      m_reTotalPixels("Total pixels: (\\d+)"),
      m_reActivePixels("Actives? pixels: (\\d+)"),
      m_reTotalLines("Total lines: (\\d+)"),
      m_reActiveLines("Actives? lines: (\\d+)"),
      m_reColorCorrection("Color gamma is (on|off)"),
      // Some VM1 firmware version changed this
      //   SDRAM status \d+ / \d+
      // to:
      //   SDRAM status: \d+ / eye: \d+
      //
      // At least VM1 version 3.3 uses the latter format.
      m_reSdram("SDRAM status:? (\\d+) / (eye: )?(\\d+)"),
      m_reHelpHeader("Available commands:"),
      m_reCmdHelpMsg("[a-z0-9]\\. [A-Z].*"),
      m_reHelpMsg("(Enter screensaver time in minutes.*|\\(c\\) .* by MultiTouch|"
                  "Type \\? to display available commands\\.|Color gamma load start|"
                  "Switching to binary mode|Color gamma load end|"
                  "Returning to text mode)"),
      m_reSelect("(DVI1|DVI2|Colorbar|Logo|Screensaver) selected"),
      m_reBoot("(Warm|Cold) boot"),
      m_reInit("(Initialize IO|Initialize DVI|Copy EDID|Set LEDs|Copy logo|Load EEPROM|Power LCD|Clear timer)... ok"),
      m_reFrameRate("Set ([0-9.]+) Hz frame rate"),
      m_reFailToLock("Failed to lock to DVI input"),
      m_reClockLost("clock lost"),
      m_requestReconnect(false)
  {
    // These are required so that Mushy serialization works
    m_autoSelect.setAllowIntegers(true);
    m_priorityVideoSource.setAllowIntegers(true);
    m_statusExternalDVI.setAllowIntegers(true);
    m_statusInternalDVI.setAllowIntegers(true);
    m_activeVideoSource.setAllowIntegers(true);
    m_colorCorrectionEnabled.setAllowIntegers(true);

    m_autoSelect.addListener([=] {
      if (m_listenersEnabled) {
        if (*m_autoSelect == MAYBE_TRUE) {
          queueWrite("a");
        } else if (*m_autoSelect == MAYBE_FALSE) {
          // special case when the screen is blank. There is no way to change
          // the state without showing something. Let's put the logo on.
          if (m_activeVideoSource == SOURCE_NONE)
            queueWrite(QByteArray::number(SOURCE_LOGO));
          else
            queueWrite(QByteArray::number(*m_activeVideoSource));
        }
      }
    });

    m_activeVideoSource.addListener([=] {
      if (m_listenersEnabled && m_activeVideoSource != SOURCE_NONE)
        queueWrite(QByteArray::number(*m_activeVideoSource));
    });

    m_lcdPower.addListener([=] {
      if (m_listenersEnabled)
        queueWrite(*m_lcdPower ? "o" : "f");
    });

    m_logoTimeout.addListener([=] {
      if (m_listenersEnabled) {
        QByteArray ba;
        ba += 'x';
        ba += QByteArray::number(Nimble::Math::Clamp(*m_logoTimeout, 1, 99)).rightJustified(2, '0');
        queueWrite(ba);
      }
    });

    m_priorityVideoSource.addListener([=] {
      if (m_listenersEnabled) {
        if (m_priorityVideoSource == SOURCE_EXTERNAL_DVI)
          queueWrite("y");
        else if (m_priorityVideoSource == SOURCE_INTERNAL_DVI)
          queueWrite("u");
      }
    });

    m_colorCorrectionEnabled.addListener([=] {
      if (m_listenersEnabled)
        queueWrite(*m_colorCorrectionEnabled ? "g" : "c");
    });

    // you can enable this to get prints of everything the serial
    // port reads and writes
    // m_port.setTraceName("VM1");
  }

  void VM1::D::childLoop()
  {
    if (!m_infoPoller) {
      auto weak = m_host->s_multiSingletonInstance;
      auto gen = std::make_shared<int>(-1);
      m_infoPoller = std::make_shared<Radiant::FunctionTask>([weak, gen] (Radiant::Task & t) {
        auto vm1 = weak.lock();
        if (vm1) {
          if (vm1->isConnected()) {
            if (*gen != -1 && *gen == vm1->m_d->m_headerGeneration) {
              // If the generation is the same than previously, we didn't get any reply in 20
              // seconds, so there is something wrong with the connection. For example someone
              // could have opened the same device elsewhere which would have invalidated our
              // handle. Reopen it and try again.
              vm1->reconnect();
            } else {
              *gen = vm1->m_d->m_headerGeneration;
              vm1->write("i");
            }
          }
          t.scheduleFromNowSecs(20.0);
        } else {
          t.setFinished();
        }
      });
      m_infoPoller->scheduleFromNowSecs(20.0);
      Radiant::BGThread::instance()->addTask(m_infoPoller);
    }

    m_startTime = Radiant::TimeStamp::currentTime();
    int openFailures = 0;
    while (m_running) {
      if(m_requestReconnect) {
        m_requestReconnect = false;
        closePort();
      }

      if (!m_port.isOpen()) {
        QByteArray buffer;
        bool ok = open();
        if (ok) {
          // We have just opened a device, make sure its VM1. It has 5 seconds
          // time to respond.
          Radiant::Timer timer;
          const double timeout = 5.0;
          m_port.write("i", 1);

          ok = false;

          while (m_port.isOpen()) {
            const double timeRemaining = timeout - timer.time();
            if (timeRemaining <= 0.0)
              break;
            bool readOk = m_port.read(buffer, timeRemaining);
            if(!readOk) {
              closePort();
            }
            if (buffer.contains("Firmware version")) {
              Radiant::info("Found VM1 output at %s", m_device.toUtf8().data());
              ok = true;
              break;
            }
          }
        }

        opened(ok);
        if (ok) {
          m_deviceCandidates.clear();
          openFailures = 0;
          {
            Radiant::Guard g(m_connectedMutex);
            m_connected = true;
            m_connectedCondition.wakeAll();
          }
          Radiant::Guard g(m_readBufferMutex);
          m_readBuffer += buffer;
          scheduleUpdate();
        } else {
          if (m_device.isEmpty()) {
            Radiant::error("Failed to detect VM1");
          } else {
            Radiant::error("Failed to open VM1 at %s", m_device.toUtf8().data());
          }
          m_port.close();

          if (m_deviceCandidates.isEmpty()) {
            ++openFailures;
          }

          if (openFailures == 5) {
            this->sleep(120);
            openFailures = 0;
            continue;
          }
          this->sleep(2);
          continue;
        }
      }

      writeColorCorrection();
      runTasks();

      QByteArray buffer;
      bool ok = m_port.read(buffer, 20.0);
      if(!ok) {
        closePort();
      }
      if (!buffer.isEmpty()) {
        Radiant::Guard g(m_readBufferMutex);
        m_readBuffer += buffer;
        scheduleUpdate();
      }

      buffer.clear();
      {
        Radiant::Guard g(m_writeBufferMutex);
        std::swap(buffer, m_writeBuffer);
      }
      if (!buffer.isEmpty()) {
        bool ok = false;
        int written = m_port.write(buffer.data(), buffer.size(), 1, &ok);
        if(!ok) {
          closePort();
        }
        if (buffer.size() != written) {
          Radiant::Guard g(m_writeBufferMutex);
          m_writeBuffer = buffer.mid(written) + m_writeBuffer;
        }
      }
    }
    closePort();
    m_readBuffer.clear();
    m_writeBuffer.clear();
  }

  void VM1::D::sleep(double seconds)
  {
    Radiant::Timer t;
    while (m_running) {
      const double shouldSleep = seconds - t.time();
      if (shouldSleep < 0)
        return;
      Radiant::Sleep::sleepSome(std::min(shouldSleep, 0.2));
    }
  }


  QStringList VM1::D::findVM1()
  {
    if (!s_enabled) return QStringList();
#ifdef RADIANT_LINUX
    QDir dir("/dev");
    QStringList tmp = dir.entryList(QStringList() << "ttyUSB*", QDir::System);
    for (auto & str: tmp) str.insert(0, "/dev/");
    tmp.insert(0, "/dev/ttyVM1");
    return tmp;
#elif defined(RADIANT_WINDOWS)
    return Radiant::SerialPort::scan();
#else
    return QStringList();
#endif
  }

  bool VM1::D::open()
  {
    if (m_deviceCandidates.isEmpty())
      m_deviceCandidates = findVM1();

    if (m_deviceCandidates.isEmpty())
      return false;

    m_device = m_deviceCandidates.takeFirst();
    assert(!m_device.isEmpty());

    return m_port.open(m_device.toUtf8().data(), false, false, 115200, 8, 1, 30000);
  }

  void VM1::D::opened(bool ok)
  {
#ifdef RADIANT_WINDOWS
    QSettings settings("SOFTWARE\\MultiTouch\\MTSvc", QSettings::NativeFormat);
    if (ok) {
      settings.setValue("VM1", m_device);
    } else {
      if (settings.value("VM1") == m_device) {
        Radiant::warning("Failed to open VM1, clearing VM1 device name");
        settings.setValue("VM1", "");
      }
    }
#else
    (void)ok;
#endif
  }

  void VM1::D::closePort()
  {
    m_port.close();
    m_connected = false;
  }

  void VM1::D::writeColorCorrection()
  {
    // VM1 seems to enable color correction automatically when it is written,
    // so refuse writing it until the correction is enabled
    if(!*m_colorCorrectionEnabled)
      return;

    QByteArray data;
    {
      Radiant::Guard g(m_colorCorrectionMutex);
      std::swap(data, m_colorCorrection);
    }
    if (!data.isEmpty()) {
      bool ok;
      m_port.write(data, 5.0, &ok);
      if (!ok) {
        Radiant::error("VM1: Failed to write color correction");
        Radiant::Guard g(m_colorCorrectionMutex);
        if (m_colorCorrection.isEmpty())
          m_colorCorrection = data;
      } else if (m_useColorCorrectionDelay) {
        this->sleep(0.1); /// Do not remove this. Can mess VM1 pretty well
        queueWrite(*m_colorCorrectionEnabled ? "g" : "c");
      }
    }
  }

  void VM1::D::runTasks()
  {
    while(m_port.isOpen()) {
      VM1Task task;
      {
        Radiant::Guard guard(m_taskMutex);
        if(m_tasks.empty()) {
          return;
        }
        task = m_tasks.front();
        m_tasks.pop_front();
      }
      if(task) {
        task(m_port);
      }
    }
  }

  void VM1::D::queueWrite(const QByteArray &data)
  {
    {
      Radiant::Guard g(m_writeBufferMutex);
      m_writeBuffer.append(data);
      if(m_connected) {
        // only interrupt read if we are connected. Else it doesn't help at all,
        // and might delay discovery of VM1
        m_port.interruptRead();
      }
    }
  }

  // Extract lines that have \n in the end from m_buffer, returned strings don't have \n
  QList<QByteArray> VM1::D::takeLines()
  {
    Radiant::Guard g(m_readBufferMutex);
    m_updateScheduled = false;
    QList<QByteArray> lines;
    for (int i = 0; i < m_readBuffer.size();) {
      int end = m_readBuffer.indexOf('\n', i);
      if (end < 0) {
        m_readBuffer = m_readBuffer.mid(i);
        return lines;
      }
      if (end > i) {
        // Split strings like "DVI2 disconnected. Logo selected" to two.
        QByteArray tmp = m_readBuffer.mid(i, end-i).trimmed();

        /// At least in initialization, VM1 sends some null bytes for unknown reason
        bool isNull = true;
        for (int i = 0; i < tmp.size(); ++i) {
          if (tmp[i] != '\0') {
            isNull = false;
            break;
          }
        }
        if (!isNull) {
          QList<QByteArray> lst;
          for (auto s: QString(tmp).split(". ")) {
            QByteArray trimmed = s.toUtf8().trimmed();
            // Do not split lines like "x. Set screensaver timeout"
            if (trimmed.size() < 5) {
              lst.clear();
              break;
            }
            lst << trimmed;
          }
          if (lst.isEmpty() && !tmp.isEmpty())
            lst << tmp;
          lines += lst;
        }
      }
      i = end + 1;
    }
    m_readBuffer.clear();
    return lines;
  }

  void VM1::D::processBuffer()
  {
    m_listenersEnabled = false;
    for (QByteArray lineRaw: takeLines()) {
      const QString line = lineRaw;

      if (m_parsingHelp && m_reCmdHelpMsg.exactMatch(line))
        continue;
      m_parsingHelp = false;

      if (m_reHelpHeader.exactMatch(line)) {
        m_parsingHelp = true;
      } else if (m_reHeader.exactMatch(line)) {
        ++m_headerGeneration;
        m_unknownLines = QStringList();
      } else if (m_reVersion.exactMatch(line)) {
        m_version = m_reVersion.cap(1);
        QRegExp v("^([01]\\.|2\\.[0-4])");
        /// VM1 Firmware version 2.4 turns off the "Color gamma" mode
        /// while reading the new color table, meaning that if we always
        /// immediately set the new color table when moving a slider in GUI,
        /// all we get is a blinking screen without the color correction.
        /// .. with this delay we actually can see something.
        m_useColorCorrectionDelay = v.indexIn(*m_version);
      } else if (m_reBoard.exactMatch(line)) {
        m_boardRevision = m_reBoard.cap(1);
      } else if (m_reAutosel.exactMatch(line)) {
        m_autoSelect = m_reAutosel.cap(1) == "on" ? MAYBE_TRUE : MAYBE_FALSE;
      } else if (m_rePriority.exactMatch(line)) {
        const bool ext = m_rePriority.cap(1) == "DVI1";
        m_priorityVideoSource = ext ? SOURCE_EXTERNAL_DVI : SOURCE_INTERNAL_DVI;
      } else if (m_reNotConnected.exactMatch(line)) {
        const bool ext = m_reNotConnected.cap(1) == "DVI1";
        (ext ? m_statusExternalDVI : m_statusInternalDVI) = STATUS_NOT_CONNECTED;
        if (m_activeVideoSource == (ext ? SOURCE_EXTERNAL_DVI : SOURCE_INTERNAL_DVI))
          m_activeVideoSource = SOURCE_NONE;
      } else if (m_reDetected.exactMatch(line)) {
        const bool ext = m_reDetected.cap(1) == "DVI1";
        (ext ? m_statusExternalDVI : m_statusInternalDVI) = STATUS_DETECTED;
      } else if (m_reActive.exactMatch(line)) {
        const bool ext = m_reActive.cap(1) == "DVI1";
        (ext ? m_statusExternalDVI : m_statusInternalDVI) = STATUS_ACTIVE;
      } else if (m_rePixelsLines.exactMatch(line)) {
        m_totalSize = Nimble::Vector2i(m_rePixelsLines.cap(1).toInt(), m_rePixelsLines.cap(3).toInt());
        m_activeSize = Nimble::Vector2i(m_rePixelsLines.cap(2).toInt(), m_rePixelsLines.cap(4).toInt());
      } else if (m_reUptime.exactMatch(line)) {
        const int mins = m_reUptime.cap(1).toInt() * 60 + m_reUptime.cap(2).toInt();
        auto ts = Radiant::TimeStamp::currentTime() - Radiant::TimeStamp::createSeconds(mins * 60);
        if (std::abs((*m_bootTime - ts).secondsD()) > 90)
          m_bootTime = ts;
      } else if (m_reScreensaver.exactMatch(line)) {
        m_logoTimeout = m_reScreensaver.cap(1).toInt();
      } else if (m_reTemp.exactMatch(line)) {
        m_temperature = m_reTemp.cap(1).toInt();
        m_temperatureTimestamp = Radiant::TimeStamp::currentTime();
      } else if (m_reSrc.exactMatch(line)) {
        if (m_reSrc.cap(1) == "DVI1") {
          m_activeVideoSource = SOURCE_EXTERNAL_DVI;
          m_statusExternalDVI = STATUS_ACTIVE;
        } else if (m_reSrc.cap(1) == "DVI2") {
          m_activeVideoSource = SOURCE_INTERNAL_DVI;
          m_statusInternalDVI = STATUS_ACTIVE;
        } else if (m_reSrc.cap(1) == "colorbar") {
          m_activeVideoSource = SOURCE_TEST_IMAGE;
        } else {
          m_activeVideoSource = SOURCE_LOGO;
        }
      } else if (m_reTotalPixels.exactMatch(line)) {
        m_totalSize = Nimble::Vector2i(m_reTotalPixels.cap(1).toInt(), m_totalSize->y);
      } else if (m_reActivePixels.exactMatch(line)) {
        m_activeSize = Nimble::Vector2i(m_reActivePixels.cap(1).toInt(), m_activeSize->y);
      } else if (m_reTotalLines.exactMatch(line)) {
        m_totalSize = Nimble::Vector2i(m_totalSize->x, m_reTotalLines.cap(1).toInt());
      } else if (m_reActiveLines.exactMatch(line)) {
        m_activeSize = Nimble::Vector2i(m_activeSize->x, m_reActiveLines.cap(1).toInt());
      } else if (m_reColorCorrection.exactMatch(line)) {
        m_colorCorrectionEnabled = m_reColorCorrection.cap(1) == "on" ? MAYBE_TRUE : MAYBE_FALSE;
      } else if (m_reSdram.exactMatch(line)) {
        m_sdramStatus = m_reSdram.cap(1).toInt();
        m_sdramTotal = m_reSdram.cap(3).toInt();
      } else if (m_reHelpMsg.exactMatch(line)) {
        // do nothing
      } else if (m_reSelect.exactMatch(line)) {
        if (m_reSelect.cap(1) == "DVI1") {
          m_activeVideoSource = SOURCE_EXTERNAL_DVI;
          m_statusExternalDVI = STATUS_ACTIVE;
        } else if (m_reSelect.cap(1) == "DVI2") {
          m_activeVideoSource = SOURCE_INTERNAL_DVI;
          m_statusInternalDVI = STATUS_ACTIVE;
        } else if (m_reSelect.cap(1) == "Colorbar") {
          m_activeVideoSource = SOURCE_TEST_IMAGE;
        } else if (m_reSelect.cap(1) == "Screensaver") {
          m_activeVideoSource = SOURCE_SCREENSAVER;
        } else {
          m_activeVideoSource = SOURCE_LOGO;
        }
      } else if (m_reBoot.exactMatch(line)) {
        Radiant::info("VM1: %s", lineRaw.data());
        /// VM1 was booted, update info right away
        m_infoPoller->scheduleFromNowSecs(0);
        Radiant::BGThread::instance()->reschedule(m_infoPoller);
      } else if (m_reInit.exactMatch(line)) {
        m_lcdPower = true;
        Radiant::info("VM1: %s", lineRaw.data());
      } else if (m_reFrameRate.exactMatch(line)) {
        m_frameRate = m_reFrameRate.cap(1).toFloat();
      } else if(m_reFailToLock.exactMatch(line) || m_reClockLost.indexIn(line) >= 0) {
        /// We don't know what this means, but did not seem to matter so
        /// print only warning
        Radiant::warning("VM1: %s", lineRaw.data());
      } else {
        auto lst = *m_unknownLines;
        lst << line;
        m_unknownLines = lst;
        Radiant::error("VM1: %s", lineRaw.data());
      }
    }
    m_listenersEnabled = true;
  }

  void VM1::D::scheduleUpdate()
  {
    if (!m_updateScheduled) {
      m_updateScheduled = true;
      auto weak = m_host->s_multiSingletonInstance;
      Valuable::Node::invokeAfterUpdate([weak] {
        auto vm1 = weak.lock();
        if (vm1)
          vm1->m_d->processBuffer();
      });
    }
  }

  VM1::VM1()
    : m_d(new D(this))
  {
  }

  void VM1::run()
  {
    if (s_enabled && !m_d->isRunning()) m_d->run();
  }

  VM1::~VM1()
  {
    m_d->m_running = false;
    m_d->m_connected = false;
    m_d->m_port.interruptRead();
    // Do not close the serial port before the thread has finished, otherwise
    // we will both call close at the same time at cause an exception on windows
    m_d->waitEnd();
    // don't interrupt writes, it messes with VM1
    m_d->m_port.close();
  }

  bool VM1::waitForConnection(double timeoutFromBeginningSecs) const
  {
    if (m_d->m_connected)
      return true;

    double timeoutSecs = timeoutFromBeginningSecs - m_d->m_startTime.sinceSecondsD();

    if (timeoutSecs <= 0.0)
      return false;

    unsigned int timeoutMs = std::max<unsigned int>(1, timeoutSecs * 1000.0);

    Radiant::Guard g(m_d->m_connectedMutex);

    while (timeoutMs > 0 && !m_d->m_connected)
      m_d->m_connectedCondition.wait2(m_d->m_connectedMutex, timeoutMs);

    return m_d->m_connected;
  }

  bool VM1::isConnected() const
  {
    return m_d->m_connected;
  }

  QString VM1::version() const
  {
    return m_d->m_version;
  }

  QString VM1::boardRevision() const
  {
    return m_d->m_boardRevision;
  }

  VM1::Maybe VM1::isAutoselectEnabled() const
  {
    return m_d->m_autoSelect;
  }

  VM1::VideoSource VM1::priorityVideoSource() const
  {
    return m_d->m_priorityVideoSource;
  }

  VM1::SourceStatus VM1::videoSourceStatus(VM1::VideoSource src) const
  {
    if (src == SOURCE_EXTERNAL_DVI)
      return m_d->m_statusExternalDVI;
    else if (src == SOURCE_INTERNAL_DVI)
      return m_d->m_statusInternalDVI;
    else return m_d->m_activeVideoSource == src ? STATUS_ACTIVE : STATUS_UNKNOWN;
  }

  VM1::VideoSource VM1::activeVideoSource() const
  {
    return m_d->m_activeVideoSource;
  }

  Nimble::Vector2i VM1::totalSize() const
  {
    return m_d->m_totalSize;
  }

  Nimble::Vector2i VM1::activeSize() const
  {
    return m_d->m_activeSize;
  }

  Radiant::TimeStamp VM1::bootTime() const
  {
    return m_d->m_bootTime;
  }

  int VM1::logoTimeout() const
  {
    return m_d->m_logoTimeout;
  }

  int VM1::temperature(Radiant::TimeStamp * timestamp) const
  {
    if (timestamp)
      *timestamp = m_d->m_temperatureTimestamp;
    return m_d->m_temperature;
  }

  VM1::Maybe VM1::isColorCorrectionEnabled() const
  {
    return m_d->m_colorCorrectionEnabled;
  }

  int VM1::sdramStatus() const
  {
    return m_d->m_sdramStatus;
  }

  int VM1::sdramTotal() const
  {
    return m_d->m_sdramTotal;
  }

  QStringList VM1::unknownLines()
  {
    return m_d->m_unknownLines;
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

    Radiant::Guard g(m_d->m_colorCorrectionMutex);
    m_d->m_colorCorrection = ba;
    if (m_d->m_connected)
      m_d->m_port.interruptRead();
  }

  void VM1::setLCDPower(bool enable)
  {
    m_d->m_lcdPower = enable;
  }

  void VM1::setLogoTimeout(int timeoutMins)
  {
    m_d->m_logoTimeout = timeoutMins;
  }

  void VM1::setAutoselect(bool enabled)
  {
    m_d->m_autoSelect = enabled ? MAYBE_TRUE : MAYBE_FALSE;
  }

  void VM1::setActiveVideoSource(VM1::VideoSource src)
  {
    m_d->m_activeVideoSource = src;
  }

  void VM1::setPriorityVideoSource(VM1::VideoSource src)
  {
    m_d->m_priorityVideoSource = src;
  }

  void VM1::setColorCorrectionEnabled(bool enabled)
  {
    m_d->m_colorCorrectionEnabled = enabled ? MAYBE_TRUE : MAYBE_FALSE;
  }

  void VM1::write(const QByteArray & data)
  {
    m_d->queueWrite(data);
  }

  void VM1::scheduleTask(const VM1Task & task)
  {
    {
      Radiant::Guard guard(m_d->m_taskMutex);
      m_d->m_tasks.push_back(task);
    }

    // Blocking while reading can delay tasks for no reason. You should not
    // wait for a read timeout before a task starts running. After the task
    // is done, reading will be resumed.
    if(m_d->m_connected) {
      m_d->m_port.interruptRead();
    }
  }

  void VM1::reconnect()
  {
    m_d->m_requestReconnect = true;
  }

  bool VM1::enabled()
  {
    return s_enabled;
  }

  void VM1::setEnabled(bool enabled)
  {
    s_enabled = enabled;
  }

  // Initialization requires s_multiSingletonInstance to be initialized
  DEFINE_SINGLETON2(VM1, , p->run();)
}
