#ifndef LUMINOUS_XRANDR_HPP
#define LUMINOUS_XRANDR_HPP

/// @cond

#include "Export.hpp"

#include <Nimble/Rect.hpp>

class QString;
namespace Luminous
{
  class LUMINOUS_API XRandR
  {
  public:
    XRandR();
    bool getGeometry(int screen, const QString & display, Nimble::Recti & rect);
  };
}

/// @endcond

#endif // LUMINOUS_XRANDR_HPP
