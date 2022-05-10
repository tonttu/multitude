#include "VideoCaptureMonitor.hpp"

#import <AVFoundation/AVFoundation.h>

namespace VideoDisplay
{
  struct Source
  {
    QString name;
    Nimble::Vector2i resolution{0, 0};

    QByteArray device() const
    {
      return QString("AVFoundation:%1").arg(name).toUtf8();
    }
  };

  class VideoCaptureMonitor::D
  {
  public:
    Radiant::Mutex m_sourcesMutex;
    std::vector<Source> m_sources;

    double m_pollInterval = 1.0;
  };

  VideoCaptureMonitor::VideoCaptureMonitor()
    : m_d(new D())
  {
    eventAddOut("source-added");
    eventAddOut("source-removed");
    eventAddOut("resolution-changed");
  }

  VideoCaptureMonitor::~VideoCaptureMonitor()
  {}

  double VideoCaptureMonitor::pollInterval() const
  {
    return m_d->m_pollInterval;
  }

  void VideoCaptureMonitor::setPollInterval(double seconds)
  {
    m_d->m_pollInterval = seconds;
    if (secondsUntilScheduled() > 0) {
      scheduleFromNowSecs(m_d->m_pollInterval);
    }
  }

  void VideoCaptureMonitor::doTask()
  {
    QMacAutoReleasePool pool;
    NSArray * devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    Radiant::Guard g(m_d->m_sourcesMutex);

    std::vector<QString> seen;
    for (AVCaptureDevice * dev: devices) {
      const char * name = [[dev localizedName] UTF8String];
      AVCaptureDeviceFormat * format = [dev activeFormat];
      if (format) {
        auto res = CMVideoFormatDescriptionGetDimensions([format formatDescription]);
        Nimble::Vector2i resolution(res.width, res.height);
        bool found = false;
        for (Source & s: m_d->m_sources) {
          if (s.name == name) {
            if (s.resolution != resolution) {
              s.resolution = resolution;
              eventSend("resolution-changed", s.device(), resolution);
            }
            found = true;
            seen.push_back(s.name);
          }
        }
        if (!found) {
          Source s;
          s.name = name;
          s.resolution = resolution;
          m_d->m_sources.push_back(s);
          seen.push_back(s.name);
          eventSend("source-added", s.device(), resolution);
        }
      }
    }

    for (auto it = m_d->m_sources.begin(); it != m_d->m_sources.end();) {
      if (std::find(seen.begin(), seen.end(), it->name) == seen.end()) {
        eventSend("source-removed", it->device());
        it = m_d->m_sources.erase(it);
      } else {
        ++it;
      }
    }

    scheduleFromNowSecs(pollInterval());
  }

  void VideoCaptureMonitor::addHint(const QString&)
  {}

  void VideoCaptureMonitor::removeSource(const QString&)
  {}

  QList<VideoCaptureMonitor::VideoSource> VideoCaptureMonitor::sources() const
  {
    QList<VideoSource> ret;
    Radiant::Guard g(m_d->m_sourcesMutex);
    for (auto & s: m_d->m_sources) {
      VideoSource vs;
      vs.device = s.device();
      vs.resolution = s.resolution;
      vs.friendlyName = s.name;
      ret << vs;
    }
    return ret;
  }

  DEFINE_SINGLETON(VideoCaptureMonitor)
}
