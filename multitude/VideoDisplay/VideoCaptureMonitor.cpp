#include "VideoCaptureMonitor.hpp"

#if defined(RADIANT_LINUX)
#include "V4L2Monitor.cpp"
#elif defined(RADIANT_WINDOWS)
#include "WindowsVideoMonitor.cpp"
#endif

#if defined(RADIANT_LINUX) || defined(RADIANT_WINDOWS)
namespace VideoDisplay
{
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

  void VideoCaptureMonitor::setPollInterval(double seconds)
  {
    m_d->m_pollInterval = seconds;
    if (secondsUntilScheduled() > 0) {
      scheduleFromNowSecs(m_d->m_pollInterval);
    }
  }

  void VideoCaptureMonitor::doTask()
  {
    if (!m_d->m_initialized) {
      m_d->m_initialized = true;
      if (!m_d->init()) {
        setFinished();
        return;
      }
    }

    m_d->poll();
    scheduleFromNowSecs(m_d->m_pollInterval);
  }

}
#endif
