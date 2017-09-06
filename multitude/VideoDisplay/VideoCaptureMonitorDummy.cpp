#include "VideoCaptureMonitor.hpp"

namespace VideoDisplay {

  class VideoCaptureMonitor::D
  {};

  VideoCaptureMonitor::VideoCaptureMonitor()
  {
    eventAddOut("source-added");
    eventAddOut("source-removed");
    eventAddOut("resolution-changed");
  }

  VideoCaptureMonitor::~VideoCaptureMonitor()
  {}

  double VideoCaptureMonitor::pollInterval() const
  {
    return 10.0f;
  }

  void VideoCaptureMonitor::setPollInterval(double)
  {}

  void VideoCaptureMonitor::doTask()
  {
    scheduleFromNowSecs(pollInterval());
  }

  DEFINE_SINGLETON(VideoCaptureMonitor);
}
