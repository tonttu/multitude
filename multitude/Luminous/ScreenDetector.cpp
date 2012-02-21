#include "ScreenDetector.hpp"

#include "ScreenDetectorAMD.hpp"

#ifdef RADIANT_LINUX
#include "ScreenDetectorNV.hpp"
#include <QX11Info>
#include <X11/Xlib.h>
#endif

namespace Luminous
{

#ifdef RADIANT_LINUX
  void ScreenDetector::scan(bool /*forceRescan*/)
  {
    m_results.clear();

    ScreenDetectorNV nv;
    ScreenDetectorAMD amd;

    int screens = XScreenCount(QX11Info::display());
    for(int screen = 0; screen < screens; ++screen) {
      if(nv.detect(screen, m_results)) continue;
      if(amd.detect(screen, m_results)) continue;
    }
  }
#elif RADIANT_WINDOWS
  void ScreenDetector::scan(bool /*forceRescan*/)
  {
    m_results.clear();
  }
#else
  void ScreenDetector::scan(bool /*forceRescan*/)
  {
    m_results.clear();
  }
#endif

}
