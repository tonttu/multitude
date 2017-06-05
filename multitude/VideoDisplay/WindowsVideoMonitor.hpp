#pragma once

#include "VideoCaptureMonitor.hpp"

#include <Radiant/Task.hpp>

namespace VideoDisplay
{
  class WindowsVideoMonitor : public VideoCaptureMonitor
  {
  public:
    WindowsVideoMonitor();
    ~WindowsVideoMonitor();

  private:
    virtual bool init() override;
    virtual void poll() override;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

}
