#include "VideoCaptureMonitor.hpp"

#include <Radiant/Trace.hpp>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include <glob.h>

#include <linux/videodev2.h>

#include <QFile>

namespace VideoDisplay
{
  static const int s_failedDevicePollInterval = 5;

  // This is hard-coded in rgb133 driver
  static const QByteArray s_rgb133CtrlDevice = "/dev/video63";

  bool isRgb133CtrlDevice(const QByteArray & device)
  {
    if (device == s_rgb133CtrlDevice) {
      static bool checked = false;
      static bool isRgb133 = false;

      // This might be a RGB133 control device. Confirm this by checking that
      // the device is owned by rgb133 driver.
      if (!checked) {
        QFile file("/sys/class/video4linux/" + s_rgb133CtrlDevice.split('/').last() + "/name");
        if (file.open(QFile::ReadOnly)) {
          isRgb133 = file.readLine(255).contains("rgb133");
        }
        checked = true;
      }
      return isRgb133;
    } else {
      return false;
    }
  }

  struct Source
  {
    ~Source();

    QString name;
    QByteArray device;
    int fd{-1};
    int pollCounter{0};
    bool enabled{false};
    Nimble::Vector2i resolution{0, 0};
    bool tag{true}; // used in scanNewSources
    bool invalid{false};
    bool openFailed{false};
    bool queryDeviceFailed{false};
    bool queryInputFailed{false};
    bool queryStatusFailed{false};
  };

  Source::~Source()
  {
    if (fd >= 0) {
      close(fd);
    }
  }

  // -----------------------------------------------------------------

  class VideoCaptureMonitor::D
  {
  public:
    D(VideoCaptureMonitor& host) : m_host(host) {}
    ~D();

    bool init();
    void poll();

    void scanNewSources();
    void scanSourceStatuses();

    bool checkIsEnabled(Source & s);

    Radiant::Mutex m_sourcesMutex;
    std::vector<Source> m_sources;

    VideoCaptureMonitor & m_host;

    double m_pollInterval = 1.0;
  };

  // -----------------------------------------------------------------

  VideoCaptureMonitor::D::~D()
  {
  }

  bool VideoCaptureMonitor::D::init()
  {
    return true;
  }

  void VideoCaptureMonitor::D::poll()
  {
    Radiant::Guard g(m_sourcesMutex);
    scanNewSources();
    scanSourceStatuses();
  }

  void VideoCaptureMonitor::D::scanNewSources()
  {
    for (auto & s: m_sources) {
      s.tag = false;
    }

    glob_t buf;
    glob("/dev/video*", 0, nullptr, &buf);
    for (std::size_t i = 0; i < buf.gl_pathc; ++i) {
      QByteArray dev(buf.gl_pathv[i]);

      // Ignore RGB133 driver control device, since VIDIOC_QUERYCAP ioctl for
      // it generates a scary kernel warning to dmesg.
      if (isRgb133CtrlDevice(dev)) {
        continue;
      }

      bool found = false;
      for (auto & s: m_sources) {
        if (s.device == dev) {
          found = true;
          s.tag = true;
          break;
        }
      }

      if (!found) {
        Source s;
        s.device = dev;
        m_sources.push_back(std::move(s));
      }
    }
    globfree(&buf);

    for (auto it = m_sources.begin(); it != m_sources.end(); ) {
      if (!it->tag) {
        if (it->fd >= 0) {
          close(it->fd);
        }
        if (it->enabled) {
          m_host.eventSend("source-removed", it->device);
        }
        it = m_sources.erase(it);
      } else {
        ++it;
      }
    }
  }

  void VideoCaptureMonitor::D::scanSourceStatuses()
  {
    for (Source & s: m_sources) {
      bool enabled = true;

      // broken source or not V4L2 device at all
      if (s.invalid) {
        enabled = false;
      }

      // don't poll failed devices every time
      if (s.pollCounter > 0) {
        s.pollCounter--;
        enabled = false;
      }

      if (enabled && s.fd < 0) {
        errno = 0;
        s.fd = open(s.device.data(), O_RDWR);
        if (s.fd < 0) {
          if (!s.openFailed) {
            Radiant::error("V4L2Monitor::scanSourceStatuses # Failed to open %s: %s", s.device.data(), strerror(errno));
            s.openFailed = true;
          }
          s.pollCounter = s_failedDevicePollInterval;
          enabled = false;
        } else {
          // check if the source is a V4L2 device
          v4l2_capability cap;
          memset(&cap, 0, sizeof(cap));
          errno = 0;
          int err = ioctl(s.fd, VIDIOC_QUERYCAP, &cap);
          if (err) {
            if (!s.queryDeviceFailed) {
              Radiant::error("V4L2Monitor::scanSourceStatuses # Failed to query device %s: %s",
                             s.device.data(), strerror(errno));
              s.queryDeviceFailed = true;
            }
            close(s.fd);
            s.fd = -1;
            s.pollCounter = s_failedDevicePollInterval;
            enabled = false;
          }

          // This device doesn't really support anything, it's most likely a control device
          if (cap.capabilities == 0) {
            s.invalid = true;
            close(s.fd);
            s.fd = -1;
            enabled = false;
          }
        }
      }

      // poll source status
      if (enabled) {
        enabled = checkIsEnabled(s);
      }

      if (enabled) {
        struct v4l2_format format;
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(s.fd, VIDIOC_G_FMT, &format) != -1) {
          Nimble::Vector2i resolution(format.fmt.pix.width, format.fmt.pix.height);

          if (s.enabled && s.resolution != resolution) {
            Radiant::debug("Source %s (%s) resolution changed from %dx%d to %dx%d",
                           s.name.toUtf8().data(), s.device.data(),
                           s.resolution.x, s.resolution.y, resolution.x, resolution.y);
            m_host.eventSend("resolution-changed", s.device, resolution);
          }
          s.resolution = resolution;

          /// With datapath the resolution changes won't get updated if we don't close the device
          close(s.fd);
          s.fd = -1;
        }
      }

      if (s.enabled != enabled) {
        s.enabled = enabled;

        if (enabled) {
          Radiant::debug("Source %s (%s) with resolution %dx%d",
                         s.name.toUtf8().data(), s.device.data(),
                         s.resolution.x, s.resolution.y);
          m_host.eventSend("source-added", s.device, s.resolution, s.name);
        } else {
          m_host.eventSend("source-removed", s.device);
        }
      }
    }
  }

  bool VideoCaptureMonitor::D::checkIsEnabled(Source & s)
  {
    struct v4l2_input input;
    memset(&input, 0, sizeof(input));

    errno = 0;
    if (ioctl(s.fd, VIDIOC_G_INPUT, &input.index)) {
      if (!s.queryInputFailed) {
        Radiant::error("V4L2Monitor::scanSourceStatuses # Failed to query input %s: %s",
                       s.device.data(), strerror(errno));
        s.queryInputFailed = true;
      }
      input.index = 0;
    }

    if (ioctl(s.fd, VIDIOC_ENUMINPUT, &input) == -1) {
      if (!s.queryStatusFailed) {
        Radiant::error("V4L2Monitor::scanSourceStatuses # Failed to query input status %s: %s",
                       s.device.data(), strerror(errno));
        s.queryStatusFailed = true;
      }
      close(s.fd);
      s.fd = -1;
      s.pollCounter = s_failedDevicePollInterval;
      return false;
    }

    s.name = QString((const char*)input.name);
    return (input.status & (V4L2_IN_ST_NO_POWER | V4L2_IN_ST_NO_SIGNAL)) == 0;
  }

  // -----------------------------------------------------------------

  VideoCaptureMonitor::VideoCaptureMonitor()
    : m_d(new D(*this))
  {
    eventAddOut("source-added");
    eventAddOut("source-removed");
    eventAddOut("resolution-changed");
  }

  VideoCaptureMonitor::~VideoCaptureMonitor()
  {
  }

  double VideoCaptureMonitor::pollInterval() const
  {
    return m_d->m_pollInterval;
  }

  void VideoCaptureMonitor::addHint(const QString& device)
  {
    (void) device;
  }

  void VideoCaptureMonitor::setPollInterval(double seconds)
  {
    m_d->m_pollInterval = seconds;
    if (secondsUntilScheduled() > 0) {
      scheduleFromNowSecs(m_d->m_pollInterval);
    }
  }

  void VideoCaptureMonitor::removeSource(const QString &source)
  {
    (void) source;
  }

  QList<VideoCaptureMonitor::VideoSource> VideoCaptureMonitor::sources() const
  {
    QList<VideoSource> ret;
    Radiant::Guard g(m_d->m_sourcesMutex);
    for (auto & s: m_d->m_sources) {
      if (s.enabled) {
        VideoSource vs;
        vs.device = s.device;
        vs.resolution = s.resolution;
        vs.friendlyName = s.name;
        ret << vs;
      }
    }
    return ret;
  }

  void VideoCaptureMonitor::doTask()
  {
    m_d->poll();
    scheduleFromNowSecs(m_d->m_pollInterval);
  }

  DEFINE_SINGLETON(VideoCaptureMonitor);
}
