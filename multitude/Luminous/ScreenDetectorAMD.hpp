#ifndef LUMINOUS_SCREENDETECTORAMD_HPP
#define LUMINOUS_SCREENDETECTORAMD_HPP

#include "ScreenDetector.hpp"

namespace Luminous
{
  class ScreenDetectorAMD
  {
  public:
    static bool detect(int screen, QList<ScreenInfo> & results);
  };
}
#endif // LUMINOUS_SCREENDETECTORAMD_HPP
