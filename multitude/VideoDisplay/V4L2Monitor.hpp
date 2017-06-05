#pragma once

#include "VideoCaptureMonitor.hpp"

#include <Radiant/Platform.hpp>
#include <Radiant/Task.hpp>

#ifndef RADIANT_LINUX
#error "This file is only to be used under Linux"
#endif

namespace VideoDisplay
{
  class V4L2Monitor : public VideoCaptureMonitor
  {
  public:
    V4L2Monitor();
    virtual ~V4L2Monitor();

    virtual void poll() override;

    virtual void resetSource(const QByteArray & device) override;

  private:
    struct Source
    {
      ~Source();

      QByteArray name;
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

  private:
    void scanNewSources();
    void scanSourceStatuses();

    bool checkIsEnabled(Source & s);

  private:
    Radiant::Mutex s_sourcesMutex;
    std::vector<Source> m_sources;
  };

}

