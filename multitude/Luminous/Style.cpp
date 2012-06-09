#include "Style.hpp"

namespace Luminous
{

  Style::Style()
    : m_color(1, 1, 1, 1),
      m_texCoords(0, 0, 1, 1),
      m_texturing(1.0f),
      m_program(0)
  {
  }

}
