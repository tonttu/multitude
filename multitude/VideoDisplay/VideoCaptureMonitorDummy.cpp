#include "VideoCaptureMonitor.hpp"

namespace VideoDisplay {

  class VideoCaptureMonitor::D
  {};

  VideoCaptureMonitor::VideoCaptureMonitor()
  {}

  VideoCaptureMonitor::~VideoCaptureMonitor()
  {}

  double VideoCaptureMonitor::pollInterval() const
  {
    return 1.0f;
  }

  void VideoCaptureMonitor::setPollInterval(double)
  {}

  void VideoCaptureMonitor::doTask()
  {}

  }
