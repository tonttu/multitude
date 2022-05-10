#pragma once

#include <Radiant/Singleton.hpp>

#include "WindowsVideoHelpers.hpp"

namespace VideoDisplay
{
  /// Magewell capture card support for VideoCaptureMonitor for Windows
  class MWCapture
  {
    DECLARE_SINGLETON(MWCapture);
    MWCapture();

  public:
    ~MWCapture();

    void initInput(VideoInput & vi);

    std::unique_ptr<Source> createSource(const VideoInput & videoInput,
                                         const AudioInput & audioInput);
  };

} // namespace VideoDisplay
