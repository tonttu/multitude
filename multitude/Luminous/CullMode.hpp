#ifndef CULLMODE_HPP
#define CULLMODE_HPP

#include "RenderDefines.hpp"

namespace Luminous
{

  class CullMode
  {
  public:
    static CullMode Default() { return CullMode(); }

    LUMINOUS_API CullMode();
    LUMINOUS_API CullMode(bool enabled, Luminous::Face face);

    bool enabled() const { return m_enabled; }
    Luminous::Face face() const { return m_face; }

  private:
    bool m_enabled;
    Luminous::Face m_face;
  };

}

#endif // CULLMODE_HPP
