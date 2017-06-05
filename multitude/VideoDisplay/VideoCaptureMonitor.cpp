#include "VideoCaptureMonitor.hpp"

namespace VideoDisplay
{
  VideoCaptureMonitor::VideoCaptureMonitor()
  {
    eventAddOut("source-added");
    eventAddOut("source-removed");
    eventAddOut("resolution-changed");
  }

  bool VideoCaptureMonitor::init()
  {
    return true;
  }

  double VideoCaptureMonitor::pollInterval() const
  {
    return m_pollInterval;
  }

  void VideoCaptureMonitor::setPollInterval(double seconds)
  {
    m_pollInterval = seconds;
    if (secondsUntilScheduled() > 0) {
      scheduleFromNowSecs(m_pollInterval);
    }
  }

  void VideoCaptureMonitor::doTask()
  {
    if (!m_initialized) {
      m_initialized = true;
      if (!init()) {
        setFinished();
        return;
      }
    }

    poll();
    scheduleFromNowSecs(m_pollInterval);
  }

}
