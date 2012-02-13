#ifndef LUMINOUS_SCREENDETECTORNV_HPP
#define LUMINOUS_SCREENDETECTORNV_HPP

#include "ScreenDetector.hpp"

namespace Luminous
{
  class ScreenDetectorNV
  {
  public:
    ScreenDetectorNV();
    virtual bool detect(int screen, QList<ScreenInfo> & results);
  };
}

#endif // LUMINOUS_SCREENDETECTORNV_HPP
